from __future__ import annotations
from enum import Enum, auto
from string import Formatter

state_name_pat = "{}_s_{}"
output_name_pat = "{}_i_{}"
derivative_name_pat = "{}_ds_{}"

def parseDotSeparatedStringPair(input_str: str) -> tuple[str,str]:
    input_str = input_str.strip()
    # Parts can be enclosed in quotes 
    # but only the inside of quotes is considered when parsing
    
    first_str = None
    second_str = None
    start_ind = 0
    
    inQuotes: bool = False
    quoted: bool = False
    
    # Check string and extract first part of operand.
    i = 0
    while i < len(input_str):
        v = input_str[i]
        
        if v == '"':
            inQuotes = not inQuotes
            if inQuotes:
                quoted = True
            
        if not inQuotes:
            if v == ".":
                if first_str is None: 
                    if quoted:
                        str_start = start_ind+1
                        str_end = i-1
                        quoted = False
                    else:
                        str_start = start_ind
                        str_end = i
                    
                    if str_start == str_end:
                        raise ValueError('Dot separated pair {} has an empty first part.'.format(input_str))
                    
                    first_str = input_str[str_start:str_end].strip()
                    start_ind = i+1
                else:
                    raise ValueError('Dot separated pair {} has two dots outside of quotes (").'.format(input_str))
            
        i += 1
        
    if inQuotes:
        raise ValueError('Dot separated pair {} has an unmatched quote.'.format(input_str))
    
    if first_str is None:
        raise ValueError("Dot separated pair '{}' does not have a second part.".format(input_str))
    
    # Extract second part of string
    if quoted:
        str_start = start_ind+1
        str_end = i-1
        quoted = False
    else:
        str_start = start_ind
        str_end = i
    
    if str_start == str_end:
        raise ValueError('Dot separated pair {} has an empty second part.'.format(input_str))
    
    second_str = input_str[str_start:str_end].strip()
    
    return (first_str, second_str)


class Registry:
    operands: set[Operand] = None
    functions: set[Function] = None
    blocks: set[Block] = None
    
    def __init__(self) -> None:
        self.operands= set()
        self.functions= set()
        self.blocks= set()

    def addOperand(self, op: Operand) -> None:
        self.operands.add(op)
        
    def addFunction(self, fc: Function) -> None:
        self.functions.add(fc)
        
    def addBlocks(self, bl: Block) -> None:
        self.blocks.add(bl)
        
    def getOperands(self) -> set[Operand]:
        return self.operands
    
    def getFunctions(self) -> set[Function]:
        return self.functions
    
    def getBlocks(self) -> set[Block]:
        return self.blocks
    

class Operation(Enum): # Operations take one or two inputs, never more
    ADD = (auto(), "({}+{})", True)
    MUL = (auto(), "({}*{})", True)
    SUB = (auto(), "({}-{})", False)
    DIV = (auto(), "({}/{})", False)
    TANH = (auto(), "tanh({})", False)
    TANHF = (auto(), "fast_tanh({})", False)
    TANH3 = (auto(), "three_lin_tanh({})", False)
    TANH5 = (auto(), "five_lin_tanh({})", False)
    TANH15 = (auto(), "fifteen_lin_tanh({})", False)
    MAX = (auto(), "fmax({},{})", True)
    MAXT = (auto(), "(({0}) > ({1}) ? ({0}) : ({1}))", True)
    MIN = (auto(), "fmin({},{})", True)
    MINT = (auto(), "(({0}) < ({1}) ? ({0}) : ({1}))", True)
    INV = (auto(), "-({})", False)
    POW = (auto(), "pow({},{})", False)
    SIN = (auto(), "sin({})", False)
    COS = (auto(), "cos({})", False)
    ABS = (auto(), "fabs({})", False)
    MOD = (auto(), "fmod({},{})", False)
    PIRANGE = (auto(), "({0} - round(({0}*M_1_PI*0.5))*2*M_PI)", False)
    
    def __init__(self, id, opString, repeatable) -> None:
        self.id = id 
        self.opString = opString
        self.repeatable = repeatable
        
        # Call the parser (field analysis, possible extension to automatic nb input operations or variable one)
        parse_list = Formatter().parse(opString)
        self.nb_input = 0
        implicit = False
        numbered = False
        for _, num, _, _ in parse_list:
            if num is None:
                continue
            elif not num: # unumbered case ('' evaluates to false)
                implicit = True
                self.nb_input += 1
            else: # Numbered case
                try:
                    self.nb_input = max(self.nb_input, int(num)+1)
                    numbered = True
                except ValueError:
                    raise ValueError("Operation {} has format string {} which does not use automatic or manual numbering.".format(self.name, self.opString))
                
        if numbered and implicit:
            raise ValueError("Operation {} has format string {} which mix automatic and manual numbering.".format(self.name, self.opString))
        
        if self.nb_input <= 0:
            raise ValueError("Operation '{}' has 0 inputs in its formating string.".format(self.name))
        
        if self.repeatable and self.nb_input != 2:
            raise ValueError("Operation '{}' is marked repeatable but does not have exactly 2 inputs.".format(self.name))
            
            
    def baseStr(self) -> str:
        return self.name
    
    def toStr(self) -> str:
        return self.opString
        
    def fromStr(identifier: str) -> Operation:
        identifier = identifier.upper()
        try:
            return Operation[identifier]
        except KeyError:
            raise ValueError("String '{}' is not linked to a supported operarion.".format(identifier))
    
    def nbInput(self) -> int:
        return self.nb_input
    
    def isRepeatable(self) -> bool:
        return self.repeatable
        

class OperandType(Enum):
    CONST = auto() # Constant value
    NAMED_CONST = auto() # Constant value with a name that is defined at the start of the file
    INPUT = auto() # Input to the whole system
    INTERN = auto() # Result of internal computations
    STATE = auto() # Part of the state of the system
    
class Operand:
    type: OperandType = None
    index: int = None
    parent: str = None
    value: float = None
    
    def __init__(self, operand_str: str, block_name: str) -> Operand:
        
        id_str, val_str = parseDotSeparatedStringPair(operand_str)
        
        if id_str.upper() == "EXT":
            self.type = OperandType.INPUT
            self.parent = val_str
        elif id_str.upper() == "CONST":
            try:
                self.value = float(val_str)
                self.type = OperandType.CONST
            except ValueError:
                self.parent = val_str
                self.type = OperandType.NAMED_CONST
        else: 
            if id_str.upper() == "THIS":
                self.parent = block_name
            else:
                if id_str.find(" ") != -1:
                    raise ValueError("Operand '{}' has a space inside a name which is forbidden.".format(operand_str))
                self.parent = id_str
            
            if val_str[0].lower() == 's': # State variable
                self.type = OperandType.STATE
            elif val_str[0].lower() == 'i': # Internal variable
                self.type = OperandType.INTERN
            else: 
                raise ValueError("String '{}' second part ({}) does not start with 's' or 'i'.".format(operand_str, val_str))
            
            self.index = int(val_str[1:])
            
    def dependancy(self) -> str:
        if self.type is OperandType.INTERN:   
            return self.toStr()
        return None
            
    def toStr(self) -> str:
        if self.type == OperandType.INPUT or self.type == OperandType.NAMED_CONST:
            return self.parent
        if self.type == OperandType.CONST:
            return "({})".format(self.value) # Enclosing parenthesis may be unecessary but there for safety
        if self.type == OperandType.STATE:
            return state_name_pat.format(self.parent, self.index)
        if self.type == OperandType.INTERN:
            return output_name_pat.format(self.parent, self.index)
        
    def getType(self): 
        return self.type
    
    
class OperationNode:
    parent: OperationNode = None
    childs: list[OperationNode] = None
    data: Operation | Operand = None
    
    def __init__(self, parent: OperationNode = None)  -> None:
        self.parent = parent
        self.childs = []

    def setParent(self, parent: OperationNode) -> None:
        self.parent = parent
    
    def addChild(self, node: OperationNode) -> None:
        self.childs.append(node)

    def setData(self, data: Operation | Operand) -> None:
        self.data = data
        
    def getParent(self) -> OperationNode:
        return self.parent
    
    def nbChilds(self) -> int:
        return len(self.childs)
    
    def getChilds(self) -> list[OperationNode]:
        return self.childs

    def getData(self) -> Operation | Operand:
        return self.data
    
    def toStr(self) -> str:
        string_data = self.data.toStr()
        if isinstance(self.data, Operand): # Data is operand -> no child
            return string_data
        else: # Data is operator -> has childs
            if not self.data.isRepeatable() and self.nbChilds() != self.data.nbInput():
                raise ValueError("Operation {} requires {} inputs but is used with {} inputs.".format(self.data.baseStr(), self.data.nbInput(), self.nbChilds()))
            
            if self.data.isRepeatable() and self.nbChilds() < 2:
                raise ValueError("Operation {} is repeatable and requires at least 2 inputs but is used with {} inputs.".format(self.data.baseStr(), self.nbChilds()))
            
            childs_str = ['']*self.nbChilds()
            for i, opNode in enumerate(self.getChilds()):
                childs_str[i] = opNode.toStr()
            
            if self.data.isRepeatable():
                result = string_data.format(childs_str[0], childs_str[1])
                for child in childs_str[2:]:
                    result = string_data.format(result, child)
                return result
            else:
                return string_data.format(*childs_str)


class FunctionType(Enum):
    OUTPUT = 1 # Creates an output from states and inputs
    STATE = 2 # Creates a derivative of a state from states and inputs
    
class Function:
    op_tree: OperationNode = None # Top operation node of the fct
    parent_block: str = None
    type: FunctionType = None
    index: int = None
    dependencies: set[str] = None
    
    def __init__(self, function: str, type: FunctionType, index: int, block_name: str, reg: Registry) -> Function:
        self.parent_block = block_name
        self.type = type
        self.index = index
        self.dependencies = set() # What other values the function needs to be computed
        
        tree = OperationNode()
        inQuotes = False
        ascending = False # Caracterize if the last operation was closing a parenthesis 
        last_start_ind = 0
        for i, v in enumerate(function):
            if v == '"':
                inQuotes = not inQuotes
                
            if not inQuotes:
                if v == '(':
                    ascending = False
                    tree.setData(Operation.fromStr(function[last_start_ind:i].strip())) # Create operation and add it to the tree
                    
                    newTree = OperationNode(tree) # Advance deeper in the tree
                    tree.addChild(newTree)
                    tree = newTree
                    last_start_ind = i+1
                elif v == ',':
                    if not ascending: # At one of the bottoms of the tree, where an operand lies
                        op = Operand(function[last_start_ind:i].strip(), self.parent_block)
                        tree.setData(op)
                        reg.addOperand(op)
                        self.dependencies.add(op.dependancy())
                    ascending = False
                    
                    tree = tree.getParent() # Change leaf
                    if not tree.getData().isRepeatable() and tree.getData().nbInput() == tree.nbChilds():
                        raise ValueError("Operation {} only support {} input.".format( tree.getData().baseStr(), tree.getData().nbInput()))
                    newTree = OperationNode(tree) # Advance deeper in the tree
                    tree.addChild(newTree)
                    tree = newTree
                    
                    last_start_ind = i+1
                elif v == ')':
                    if not ascending: # At one of the bottoms of the tree, where an operand lies
                        op = Operand(function[last_start_ind:i].strip(), self.parent_block)
                        tree.setData(op)
                        reg.addOperand(op)
                        self.dependencies.add(op.dependancy())
                    ascending = True
                    
                    tree = tree.getParent() # Climb the tree
                    
        self.op_tree = tree
        
    def getDependencies(self) -> set[str]:
        return self.dependencies
    
    def getIndex(self) -> int:
        return self.index
    
    def outputName(self) -> str:
        if self.type == FunctionType.OUTPUT:
            return output_name_pat.format(self.parent_block, self.index+1)
        else:
            return state_name_pat.format(self.parent_block, self.index+1)
        
    def toStr(self) -> str:
        return self.op_tree.toStr()


class Block:
    name: str = None
    
    num_out: int = None
    num_state: int = None
    out_offset: int = None
    state_offset: int = None
    
    out_fcts: list[Function] = None
    state_fcts: list[Function] = None
    
    def __init__(self, name:str, out_data: tuple[int, int], state_data: tuple[int, int], functions_str: list[str], reg: Registry) -> Block:
        self.name = name 
        self.num_out, self.out_offset = out_data
        self.num_state, self.state_offset = state_data
        self.out_fcts = []
        self.state_fcts = []
        
        if self.num_out+self.num_state != len(functions_str):
            raise ValueError("Block '{}' number of state and outputs is not consistent with number of functions given.".format(self.name))
        
        for i, l in enumerate(functions_str[0:self.num_out]):
            func = Function(l, FunctionType.OUTPUT, i, self.name, reg)
            self.out_fcts.append(func)
            reg.addFunction(func)
            
        for i, l in enumerate(functions_str[self.num_out:self.num_out+self.num_state]):
            func = Function(l, FunctionType.STATE, i, self.name, reg)
            self.state_fcts.append(func)
            reg.addFunction(func)
            
    def getOutputFcts(self) -> list[Function]:
        return self.out_fcts
    
    def getStateFcts(self) -> list[Function]:
        return self.state_fcts
    
    def getName(self) -> str:
        return self.name
    
    def getOutputInfo(self) -> tuple[int, int]:
        return (self.num_out, self.out_offset)
    
    def getStateInfo(self) -> tuple[int, int]:
        return (self.num_state, self.state_offset)
            

class System:
    reg: Registry = None
    outputs: set[str] = None
    inputs: set[str] = None
    named_constants: dict[str, float] = None
    state_write_list: list[Function] = None
    output_write_list: list[Function] = None
    
    input_vec_name = "input"
    state_vec_name = "state"
    dstate_vec_name = "dstate"
    output_vec_name = "output"
    
    def __init__(self, filename: str) -> None:
        with open(filename, "r") as f:
            system_str = f.read()
        valid_names = {None}
        
        self.reg = Registry()
        self.outputs = set()
        self.inputs = set()
        self.named_constants = {}
        self.state_write_list = []
        self.output_write_list = []
        
        lines = [s.strip() for s in system_str.splitlines()] # Split by lines and remove empty lines
        lines = [s for s in lines if len(s) > 2 and s[:2] != '//']
        
        base_str = lines[0] 
        for out in base_str.split(","):
            op = Operand(out.strip(), None)
            if op.getType() == OperandType.CONST or op.getType() == OperandType.NAMED_CONST:
                raise ValueError("Constants cannot be outputs.")
            self.outputs.add(op.toStr())
        lines = lines[1:]
        
        
        inQuotes: bool = False
        hasNamedConstants: bool = True
        while hasNamedConstants:
            base_str = lines[0]
            hasNamedConstants = False
            for i in range(0,len(base_str)):
                v = base_str[i]
                if v == '"':
                    inQuotes = not inQuotes
                
                if not inQuotes:
                    if v == ".":
                        hasNamedConstants = True 
                        break
                        
            if hasNamedConstants: # Defines the name constants
                inQuotes = False
                start_ind = 0
                base_str = base_str + ","
                for i in range(len(base_str)):
                    v = base_str[i]
                    if v == '"':
                        inQuotes = not inQuotes
                    
                    if not inQuotes and v == ",":
                        id_str, val_str = parseDotSeparatedStringPair(base_str[start_ind:i])
                        
                        if val_str is None or id_str is None:
                            raise ValueError("Named constant '{}' incorrect.".format(base_str[start_ind:i]))
                        
                        if id_str.find(' ') != -1:
                            raise ValueError("Named constant '{}' has a space inside its name which is forbidden.".format(base_str[start_ind:i]))
                        
                        try:
                            self.named_constants[id_str] = float(val_str)
                        except:
                            raise ValueError("Named constant '{}' has a non numerical value.".format(base_str[start_ind:i]))
                        valid_names.add(id_str)
                        
                        start_ind = i+1
                lines = lines[1:]
                
                if inQuotes:
                    raise ValueError("Named constant line '{}' has an unmatched quote.".format(base_str))
        
        out_ind = 0
        state_ind = 0
        block_names: set[str] = set()
        while len(lines) > 0: # Parse all blocks
            quoted: bool = False
            inQuotes = False
            base_str = lines[0]
            id = -1
            # Allow quoted names for blocks (dont check for inline quote)
            for i in range(len(base_str)):
                v = base_str[i]
                if v == '"':
                    inQuotes = not inQuotes
                    quoted = True
                
                if not inQuotes:
                    if v == ",":
                        id = i 
                        break
            
            if id == -1:
                raise ValueError("Line '{}' is not a valid block starting line.".format(base_str))
            
            name = base_str[:id].strip()
            if quoted:
                name = name[1:-1]
            if name in block_names:
                raise ValueError("Name '{}' is used by two blocks.".format(name))
            if name.find(' ') != -1:
                raise ValueError("Name '{}' has a space inside which is forbidden.".format(name))
                
            block_names.add(name)
            
            # dont alloz quoted integer
            new_base_str = base_str[id+1:]
            id = new_base_str.find(',')
            if id == -1:
                raise ValueError("Line '{}' is not a valid block starting line.".format(base_str))
            num_out = int(new_base_str[:id].strip())
            num_state = int(new_base_str[id+1:].strip())
            
            self.reg.addBlocks(Block(name, (num_out, out_ind), (num_state, state_ind), lines[1:num_out+num_state+1], self.reg))
            lines = lines[num_out+num_state+1:]
            out_ind += num_out
            state_ind += num_state
            
        
        # List all possible names
        for fct in self.reg.getFunctions():
            valid_names.add(fct.outputName())
                
        for o in self.reg.getOperands():
            if o.getType() == OperandType.INPUT:
                operand_str = o.toStr()
                valid_names.add(operand_str)
                self.inputs.add(operand_str)
            if o.getType() == OperandType.NAMED_CONST:
                if o.parent not in self.named_constants.keys():
                    raise ValueError("Named constant '{}' with no defined value.".format(o.parent))
            if o.getType() == OperandType.STATE:
                operand_str = o.toStr()
                if operand_str not in valid_names:
                    raise ValueError("State '{}' is not a defined name.".format(operand_str))
        
        # Check if all the names mentioned are valid
        for out in self.outputs:
            if out not in valid_names:
                raise ValueError("Output '{}' is not a defined name.".format(out))
            
        for fct in self.reg.getFunctions():
            for name in fct.getDependencies():
                if name not in valid_names:
                    raise ValueError("Variable '{}' is not a defined name.".format(name))   

        # Compte the order of computation of internal values
        outputFct_list = []
        for b in self.reg.getBlocks():
            outputFct_list += b.getOutputFcts()
            self.state_write_list += b.getStateFcts()
        
        written_dep = {None} # Set of all functions written
        while outputFct_list:
            hasWritten = False
            for fct in outputFct_list:
                if written_dep.issuperset(fct.getDependencies()):
                    hasWritten = True
                    written_dep.add(fct.outputName())
                    self.output_write_list.append(fct)
                    outputFct_list.remove(fct)
                    break
                    
            if not hasWritten:
                raise ValueError("Computation graph of functions is cyclic. It cannot be resolved.")
            
    
    def write(self, filename: str) -> None:
        system_str = """#include <math.h>

// Algo based on use of lambert's fraction (https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/)
double fast_tanh(double x)
{
    // Clip output when it goes above 1 or below -1
	if (x < -4.971786858528029)
	{
		return -1;
	}
	if (x > 4.971786858528029)
	{
		return 1;
	}
	double x2 = x * x;
	double a = x * (135135.0 + x2 * (17325.0 + x2 * (378.0 + x2)));
	double b = 135135.0 + x2 * (62370.0 + x2 * (3150.0 + x2 * 28.0));
	return a / b;
}

double three_lin_tanh(double x)
{
    // Clip output when it goes above 1 or below -1
	if (x < -1.299725497278728)
	{
		return -1;
	}
	if (x > 1.299725497278728)
	{
		return 1;
	}
	return x*0.769393231181298;
}

double five_lin_tanh(double x)
{
    // Clip output when it goes above 1 or below -1
	if (x < -0.818631533308157)
	{   
        if (x < -1.979238406276971) {
            return -1;
        }
        return x*0.235809350838973 - 0.533277076260264;
	}
	if (x > 0.818631533308157)
	{
		if (x > 1.979238406276971) {
            return 1;
        }
        return x*0.235809350838973 + 0.533277076260264;
	}
	return x*0.887234387088490;
}

double fifteen_lin_tanh(double x)
{ 
	if (x < -0.349846806360468) {
		if (x > -1.288690112231558) {
			if (x > -0.652273105339896) {
				return 0.784454209160348 * x - 0.067136203627008;
			} else {
				if (x > -0.952698399354790) {
					return 0.558066602782327 * x - 0.214802750649666;
				} else {
					return 0.349394991344478 * x - 0.413603860857289;
				}
			}
		} else  {
			if (x > -2.289691672012657) {
				if (x > -1.703950163909747)  {
					return 0.184190996012522 * x - 0.626500616142730;
				} else { 
					return 0.073287597472899 * x - 0.815474480262468;
				}
			} else {
				if (x > -3.369261331568106) {
					return 0.015487206401210 * x - 0.947819554338388;
				} else {
					return -1;
				}
			}
		}
	} else {
		if (x < 1.288690112231558) {
			if (x < 0.652273105339896) 
				if (x < 0.349846806360468) {
					return 0.976355928445541 * x;
				} else {
					return 0.784454209160348 * x + 0.067136203627008;
				}
			else {
				if (x < 0.952698399354790) {
					return 0.558066602782327 * x + 0.214802750649666;
				} else {
					return 0.349394991344478 * x + 0.413603860857289;
				}
			}
		} else { 
			if (x < 2.289691672012657) {
				if (x < 1.703950163909747) {
					return 0.184190996012522 * x + 0.626500616142730;
				} else {
					return 0.073287597472899 * x + 0.815474480262468;
				}
			} else {
				if (x < 3.369261331568106) {
					return 0.015487206401210 * x + 0.947819554338388;
				} else {
					return 1;
				}
			}
		}
	}
}
"""+"""

/* 
 * INPUTS
 * ======
 * {0} : double vector of length {1} 
 * {2} : double vector of length {3}
 * 
 * OUTPUTS
 * =======
 * {4} : double vector of length {5}
 * {6} : double vector of length {7}
 */

void dyn_system(const double* const {0}, const double* const {2}, double* const {4}, double* const {6})
""".format(self.input_vec_name, len(self.inputs), 
           self.state_vec_name, len(self.state_write_list), 
           self.dstate_vec_name, len(self.state_write_list), 
           self.output_vec_name, len(self.outputs)) + "{\n"

        system_str += "\t// Named Constants\n"
        for name, val in self.named_constants.items():
            system_str += "\tconst double {} = {};\n".format(name, val)
        
        system_str += "\n\t// Input reading\n"
        for i, op_str in enumerate(self.inputs):
            system_str += "\tdouble {} = {}[{}];\n".format(op_str, self.input_vec_name, i)
            
        system_str += "\n\t// State reading\n"
        for b in self.reg.getBlocks():
            _, state_offset = b.getStateInfo()
            for fct in b.getStateFcts():
                system_str += "\tdouble {} = {}[{}];\n".format(fct.outputName(), self.state_vec_name, state_offset+fct.getIndex())
        
        system_str += "\n\n\t// Output Computing\n"
        for fct in self.output_write_list:
            system_str += "\tdouble {} = {};\n".format(fct.outputName(), fct.toStr())
            
        system_str += "\n\t// Derivative Computing\n"
        for b in self.reg.getBlocks():
            _, state_offset = b.getStateInfo()
            for fct in set(b.getStateFcts()):
                system_str += "\t{}[{}] = {};\n".format(self.dstate_vec_name, state_offset+fct.getIndex(), fct.toStr())
                
        system_str += "\n\n\t// Output Setting\n"
        for i, val_str in enumerate(self.outputs):
            system_str += "\t{}[{}] = {};\n".format(self.output_vec_name, i, val_str)
                
        system_str += "}"
        
        with open(filename, "w") as f:
            f.write(system_str)
    
    
if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Block converter",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-l', '--lang', choices=['c'], type = str.lower)
    parser.add_argument("src", help="Source of graph")
    parser.add_argument("dest", help="Destinqtion of zritten code")
    args = parser.parse_args()
    
    sys = System(args.src)
    sys.write(args.dest)
        