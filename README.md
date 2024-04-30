# CapoCaccia Neuromorphic Drummer Project


## Vex file 


To work in the framework of the `main.cpp` file the inclued header file must define multiple things.

First, it must define the following constants. They describe the save path of the files, the number of recorded channels in the sensory loop and the simulation loop, the length of a simulation loop and a sensory loop, the frequency of recording of the simulation and the frequency of write to the SD card in the sensory loop.
~~~C
#define BASE_FILENAME <string>

#define NB_CHANNEL_SYSTEM <int>
#define NB_CHANNEL_SENSORS <int>

#define SIM_MICRO_STEP <int>
#define SENSORS_MILLI_STEP <int>

#define SYSTEM_RECORD_PERIOD <int>
#define WRITING_PERIOD <int>
~~~

Then, it must define the following "functions" which will be called during the execution of the system. All of them return void (nothing) except the `system_update()` which must return a double corresponding to the applied voltage to the motor in mV.
~~~C
#define init_module() <whatever>()
#define create_header_file() <whatever>()
#define system_update() <whatever>()
#define system_record(count) <whatever>(count)

#define sim_setup(nb_state, nb_input, nb_output, input, state, state2, dstate, dstate2, output) \
        <whatever>(&nb_state, &nb_input, &nb_output, &input, &state, &state2, &dstate, &dstate2, &output)
#define sim_com(input, output) <whatever>(input, output)
#define sim_record(input, state, dstate, output, count) <whatever>(input, state, dstate, output, count)
#define dyn_system(input, state, dstate, output) <whatever>(input, state, dstate, output)
~~~

## Block representation of ODEs


### Structure of File


Most of the values are encoded in dot separated pairs (value1.value2) where quotes can be used to avoid problems with other art of the code.


The first line (limitation of the proccessing flow) must reprensent the outputs of the system, multiple outputs are separated by coma. The convention for referencing output follows th convention of referencing computation and state taht is described below. 


The following lines before the first block represent the named constants, multiples constants are separated by coma or line break. A named constant is represented by \<name>.\<value>.


A block starts by a defining line which is structured in the following way. First the named of the block is given then a coma followed by the number of internal computation (function) then a coma followed by the number of states.


Assuming *n* internal computations and *k* states, the *n* first lines after the block definitions represent the functions of the internal computations and *k* lines after represent the functions computing the derivatives of the states. 


A function has access to operations such as "ADD" which add two or more numbers and "TANHF" which computes an approximation of he hyperbolic tangent of a number, operations are case insensitive. The leaf of these operations are dot separated values which represent :
1. Reference to this block or another block computation or state, denoted THIS.\<val> for internal reference and \<name>.\<val> for reference to another block. \<name>.i\<num> for reference to a computation and \<name>.s\<num> for reference to a state.
2. Reference to an external input EXT.\<name> by name.
3. Reference to a constant by value with CONST.\<val> or by name (for named constant) with CONST.\<name>.


Example of the start of a file following the structure :
~~~Text
adder.i3


gfm."-2" , gsp."6" , gsm."-4" , gup."5" 
dfm."0.0" , dsp."0.5" , dsm."-0.5" , dup."-0.5" , V0."-0.85" 
tau_V_inv."1000" , tau_vf_inv."1000" , tau_vs_inv."25" , tau_vu_inv."1.25"
tau_vsyn_out_inv."25" , gsyn."-1" , dsyn."0" , tau_vsyn_inv."25"


adder, 3, 0
ADD(EXT.Iapp , ADD(synapse1.i1,EXT.feed1))
ADD(EXT.Iapp , ADD(synapse2.i1,EXT.feed2))
SUB(synapse_out_1.i1, synapse_out_2.i1)


neuron1, 4, 4
MUL( CONST.gfm , SUB( TANH(SUB(THIS.s2,CONST.dfm)) , TANH(SUB(CONST.V0, CONST.dfm)) ) )
MUL( CONST.gsp , SUB( TANH(SUB(THIS.s3,CONST.dsp)) , TANH(SUB(CONST.V0, CONST.dsp)) ) )
MUL( CONST.gsm , SUB( TANH(SUB(THIS.s3,CONST.dsm)) , TANH(SUB(CONST.V0, CONST.dsm)) ) )
MUL( CONST.gup , SUB( TANH(SUB(THIS.s4,CONST.dup)) , TANH(SUB(CONST.V0, CONST.dup)) ) )
MUL( CONST.tau_V_inv , SUB(SUB(SUB(SUB(SUB( ADD(CONST.V0,adder.i2) , THIS.i1 ), THIS.i2 ), THIS.i3 ), THIS.i4 ), THIS.s1 ))
MUL( CONST.tau_vf_inv , SUB(THIS.s1,THIS.s2) )
MUL( CONST.tau_vs_inv , SUB(THIS.s1,THIS.s3) )
MUL( CONST.tau_vu_inv , SUB(THIS.s1,THIS.s4) )
~~~


### Compile ODE file to C


~~~Bash
python convert.py <input_ode_text_file> <output_c_file>
~~~


## Read binary data file from controller 


### Compile library to read vex binary data 


The first step is to compile the reading library
~~~Bash
gcc -fPIC -shared -O2 -o read_vex_lib.so read_vex_lib.c
~~~


### Use the C read library in Julia


Somewhere in the code the declaration of the path to the library must be present.
~~~Julia
const read_lib = joinpath(@__DIR__, "read_vex_lib.so")
~~~


Then the following code can be used to translate a binary file to a matrix in Julia where the lines of the matrix correspond to the number of channels.
~~~Julia
size_vec = Ref{Culonglong}(0)
