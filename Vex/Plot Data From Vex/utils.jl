using Statistics
const read_lib = joinpath(@__DIR__, "read_vex_lib.so")


function movingaverage(X::Vector, numofele::Int)
    BackDelta = div(numofele, 2)
    ForwardDelta = isodd(numofele) ? div(numofele, 2) : div(numofele, 2) - 1
    len = length(X)
    Y = similar(X)
    for n = 1:len
        lo = max(1, n - BackDelta)
        hi = min(len, n + ForwardDelta)
        Y[n] = mean(X[lo:hi])
    end
    return Y
end


"""
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
"""

function creadToArray(file::String)
    size_vec = Ref{Culonglong}(0)
    nb_col = Ref{Culonglong}(0)

    vec_ptr = @ccall read_lib.binaryfileToVector(file::Cstring, size_vec::Ptr{Culonglong}, nb_col::Ptr{Culonglong})::Ptr{Cdouble}

    return reshape(unsafe_wrap(Vector{Float64}, vec_ptr, size_vec[], own=true), Int64(nb_col[]), :)
end