const read_lib = joinpath(@__DIR__, "lib_read_lib.so")

mutable struct Carray
    original_ptr::Union{Ptr{Cdouble},Nothing}
    array::Union{Matrix{Float64},Nothing}

    function Carray(file::String)
        size_vec = Ref{Culonglong}(0)
        nb_col = Ref{Culonglong}(0)

        vec_ptr = @ccall read_lib.binaryToVector(file::Cstring, size_vec::Ptr{Culonglong}, nb_col::Ptr{Culonglong})::Ptr{Cdouble}

        array = reshape(unsafe_wrap(Vector{Float64}, vec_ptr, size_vec[]), Int64(nb_col[]), :)
        finalizer(destroyMatrix, new(vec_ptr, array))
    end

    function destroyMatrix(mat::Carray)
        if !isnothing(mat.original_ptr)
            @ccall read_lib.freeVector(mat.original_ptr::Ptr{Cdouble})::Cvoid
        end
        mat.original_ptr = nothing # Not necessary but addes to be sure that pointer access is impossible
        mat.array = nothing
    end
end


arr = Carray("data/chart_thesis_model/long/gsm_3__gup_4__Iapp_1d5__L_long_sensors.bin")

println(arr.array[1,2])
println(size(arr.array))

arr = nothing

#sleep(5)

wait_for_key(prompt) = (print(stdout, prompt); read(stdin, 1); nothing)

wait_for_key("Cool")