# CapoCaccia Neuromorphic Drummer Project

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
