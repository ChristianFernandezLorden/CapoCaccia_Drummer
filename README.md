# CapoCaccia Neuromorphic Drummer Project

## Block representation of ODEs

### Structure of File

Example of a file following the structure :
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
nb_col = Ref{Culonglong}(0)

vec_ptr = @ccall read_lib.binaryfileToVector(<bin_file>::Cstring, size_vec::Ptr{Culonglong}, nb_col::Ptr{Culonglong})::Ptr{Cdouble}

val_matix = reshape(unsafe_wrap(Vector{Float64}, vec_ptr, size_vec[], own=true), Int64(nb_col[]), :)
~~~
