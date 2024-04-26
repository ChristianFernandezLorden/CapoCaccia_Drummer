using Statistics#
#using CSV
#using DataFrames
using Plots, Plots.PlotMeasures

include("utils.jl")

function plot_from_grid(results, keys_val, parameters_grid, metric, val_plotted, other_val)
    if length(other_val) != length(keys_val) -1
        return nothing
    end

    x_value = parameters_grid[val_plotted]
    y_value = Vector{Union{Float64, Missing}}(missing, lastindex(x_value))

    i=1
    current = fill!(Vector{Int64}(undef, length(keys_val)), 1)
    for (j, _) in enumerate(keys_val)
        if j == val_plotted
            continue
        end
        current[j] = other_val[i]
        i += 1
    end

    for i in 1:lastindex(x_value)
        current[val_plotted] = i
        out = results[current...]
        if !isempty(out)
            val = out[metric]
            if length(collect(skipmissing(val))) >= 1
                val = mean(skipmissing(val))
            else 
                val = missing
            end
            y_value[i] = val
        end
    end

    return plot(x_value, y_value, marker=:circle, markersize=2)
end


function grid_from_directory(dir, Tlim)
    files = readdir(dir)

    sensors_data = Dict{Tuple, String}()
    system_data = Dict{Tuple, String}()

    keys_val = nothing
    key_val_dict = Dict{String,Set}()
    for file in files
        params, type = parameter_from_name(file)
        if isnothing(keys_val)
            keys_val = keys(params)
            for key in keys_val
                key_val_dict[key] = Set{Float64}()
            end
        elseif keys(params) != keys_val
            println("Error all tries need to use vary the same parameters")
            return nothing
        end

        values = ();
        for key in keys_val 
            push!(key_val_dict[key], params[key])

            values = (values..., params[key]) # Unpacking then repacking
        end

        if type == "system"
            system_data[values] = file
        elseif type == "sensors"
            sensors_data[values] = file
        else
            println("WTF this was already checked")
            return nothing
        end
    end

    keys_val = collect(keys_val) # Transform to array for better reproductibility
    parameters_grid = Vector{Vector{Float64}}(undef, length(keys_val))
    size = fill!(Vector{Int64}(undef, length(keys_val)), 0)
    current = fill!(Vector{Int64}(undef, length(keys_val)), 1)
    for (i, key) in enumerate(keys_val)
        parameters_grid[i] = sort!(collect(key_val_dict[key]))
        size[i] = length(parameters_grid[i])
    end

    results = Array{Tuple, length(size)}(undef, size...)

    max_iter = prod(size)
    for i in 1:max_iter
        val_key = ()
        for (j, arr) in enumerate(parameters_grid)
            val = arr[current[j]]
            val_key = (val_key..., val)
        end

        sensor_file = sensors_data[val_key]
        system_file = system_data[val_key]

        sen_mat = creadToArray("$dir/$sensor_file")

        p_time_sen = sen_mat[1, :]
        p_angle = sen_mat[2, :]
        p_speed = sen_mat[3, :]


        sys_mat = creadToArray("$dir/$system_file")

        p_time_sys = sys_mat[1, :]
        p_neuron1 = sys_mat[2, :]
        p_neuron2 = sys_mat[3, :]

        start_T = max(p_time_sen[1], p_time_sys[1])

        sen_crop = p_time_sen .> Tlim + start_T
        sys_crop = p_time_sys .> Tlim + start_T

        out = oscillation_burst_analysis(p_time_sys[sys_crop], p_neuron1[sys_crop], p_neuron2[sys_crop], p_time_sen[sen_crop], p_angle[sen_crop], movingaverage(p_speed, 100)[sen_crop])
        if isnothing(out)
            results[current...] = ()
        else
            results[current...] = out
        end
        
        j = 1
        current[j] += 1
        while current[j] > size[j] && i < max_iter
            current[j] = 1
            j += 1
            if j > lastindex(size)
                println("Problem in update of state")
                return nothing
            end
            current[j] += 1
        end 
    end 

    return results, keys_val, parameters_grid
end

function parameter_from_name(path)
    last_back = findlast("\\", path)
    last_forw = findlast("/", path)
    if isnothing(last_back)
        last_back = 0
    else 
        last_back = last_back[end]
    end
    if isnothing(last_forw)
        last_forw = 0
    else 
        last_forw = last_forw[end]
    end
    ind_name = max(last_back, last_forw)

    str_arr = filter!(!isempty, split(path[ind_name+1:end], "_"))

    norm = false
    prev = ""
    reg_decimal = r"^(?<int>\d+)d?(?<dec>\d+)?$"
    #reg_digit = r"^\d+$"
    reg_length = r"^(?:(?<short>short)|(?<mid>mid)|(?<long>long))$"
    #rep = s"\g<int>.\g<dec>"

    param = Dict{String,Float64}()
    
    # should optimize this
    for s in str_arr 
        if norm
            m_dec = match(reg_decimal, s) # match a number representation
            if !isnothing(m_dec)
                str_val = m_dec[:int]
                if !isnothing(m_dec[:dec])
                    str_val = string(str_val, ".", m_dec[:dec])
                end

                #val = parse(Float64, replace(s, reg_decimal => rep))
                val = parse(Float64, str_val)
                param[prev] = val
                norm = false
                continue
            end

            # no match
            prev = s
        else
            norm = true 
            prev = s
        end
    end

    if str_arr[end] == "system.bin"
        type = "system"
    elseif str_arr[end] == "sensors.bin"
        type ="sensors"
    else
        println("Should not happen")
        type = nothing
    end

    return param, type
end


function oscillation_burst_analysis(T_V, Vpush, Vpull, T_theta, theta, theta_dot)
    o = oscillation_analysis(T_theta, theta, theta_dot)
    bpush = burst_analysis(T_V, Vpush)
    bpull = burst_analysis(T_V, Vpull)


    if isnothing(bpush)
        len_push = missing
        freq_push = missing
    else
        burst_time_push, bursts_size_push, len_push, freq_push = bpush;
    end

    if isnothing(bpull)
        len_pull = missing
        freq_pull = missing
    else
        burst_time_pull, bursts_size_pull, len_pull, freq_pull = bpull;
    end

    if isnothing(o)
        mean_amp = missing
        mean_period = missing
    else
        amplitude, times, mean_amp, mean_period = o;
    end

    if isnothing(bpush) || isnothing(bpull) || isnothing(o)
        phase_output = [missing, missing]
    else # Try to use 
        mid_times_push = burst_time_push .+ (bursts_size_push / 2)
        mid_times_pull = burst_time_pull .+ (bursts_size_pull / 2)

        push_osc_phase = zeros(size(mid_times_push))
        pull_osc_phase = zeros(size(mid_times_pull))

        id_push = 1
        id_pull = 1
        t_start = times[1]
        for t_end = times[2:end]
            while id_push < lastindex(mid_times_push) && mid_times_push[id_push] >= t_start && mid_times_push[id_push] < t_end
                push_osc_phase[id_push] = (mid_times_push[id_push] - t_start) / (t_end - t_start)
                id_push += 1;
            end
            while id_pull < lastindex(mid_times_pull) && mid_times_pull[id_pull] >= t_start && mid_times_pull[id_pull] < t_end
                pull_osc_phase[id_pull] = (mid_times_pull[id_pull] - t_start) / (t_end - t_start)
                id_pull += 1
            end
            t_start = t_end
        end

        phase_output = [mean(push_osc_phase), mean(pull_osc_phase)]
    end
    
    return ([len_push, len_pull], [freq_push, freq_pull], phase_output, [mean_amp], [mean_period])
end


function oscillation_analysis(T, theta, theta_dot)
    up_crossings, down_crossings = computeCrossings(T, theta)
    up_crossings_dot, down_crossings_dot= computeCrossings(T, theta_dot)

    if lastindex(up_crossings_dot) < 3 || lastindex(down_crossings_dot) < 3
        return nothing
    end

    theta_mins = zeros(size(up_crossings_dot))
    for i = 1:lastindex(up_crossings_dot)
        T_cross = up_crossings_dot[i]
        ind = lastindex(T[T .< T_cross])

        theta_mins[i] = ((T_cross - T[ind]) * theta[ind] + (T[ind+1] - T_cross) * theta[ind+1]) / (T[ind+1] - T[ind])
    end

    theta_maxs = zeros(size(down_crossings_dot))
    for i = 1:lastindex(down_crossings_dot)
        T_cross = down_crossings_dot[i]
        ind = lastindex(T[T.<T_cross])

        theta_maxs[i] = ((T_cross - T[ind]) * theta[ind] + (T[ind+1] - T_cross) * theta[ind+1]) / (T[ind+1] - T[ind])
    end

    # down_cross => max (positive to negative deriv)
    # up_cross => min (negative to positive deriv)
    if up_crossings_dot[1] < down_crossings_dot[1] # First a min
        first = theta_mins
        first_T = up_crossings_dot
        second = theta_maxs
        second_T = down_crossings_dot
    else # first a max 
        first = theta_maxs
        first_T = down_crossings_dot
        second = theta_mins
        second_T = up_crossings_dot
    end 

    # lastindex(second) = lastindex(first) or lastindex(first)-1
    nb_ind_1 = lastindex(first)#min(lastindex(first), lastindex(second))
    nb_ind_2 = lastindex(second)#min(lastindex(first) - 1, lastindex(second))

    if nb_ind_1 != nb_ind_2 && nb_ind_1 != nb_ind_2 + 1
        println("Crossing length do not agree")
        return nothing
    end

    for i = 1:min(nb_ind_1, nb_ind_2)
        if first_T[i] > second_T[i]
            println("Multiple line cross in oscillation analysis")
            return nothing
        end
    end

    for i = 1:min(nb_ind_1-1, nb_ind_2)
        if first_T[i+1] < second_T[i]
            println("Multiple line cross in oscillation analysis")
            return nothing
        end
    end
    
    amplitude = Vector{Union{Float64,Missing}}(missing, nb_ind_1 + nb_ind_2)
    #times = Array{Tuple{Float64,Float64}}(undef, nb_ind_1 + nb_ind_2, 1)
    times = Vector{Union{Float64,Missing}}(missing, nb_ind_1 + nb_ind_2)

    for i = 1:nb_ind_1
        amplitude[(2*i) - 1] = first[i]
        #times[(2*i) - 1] = (first_T[i], second_T[i])
        times[(2*i)-1] = first_T[i]
    end

    for i = 1:nb_ind_2
        amplitude[2*i] = second[i]
        #times[2*i] = (second_T[i], first_T[i+1])
        times[2*i] = second_T[i]
    end

    period = times[2:end] - times[1:end-1]

    return (amplitude, times, mean(abs.(skipmissing(amplitude))), mean(skipmissing(period)))
end


function burst_analysis(T, V)
    plateau = false;

    up_crossings, down_crossings = computeCrossings(T, V)
    
    if length(up_crossings) > 3 && length(down_crossings) > 3
        # down separation -> between a down and an up crossing
        # up separation -> between an up and a down crossing

        down_sep, down_sep_start, up_sep, up_sep_start = upanddownFromCrossings(up_crossings, down_crossings);
        
        # Test bursting on the down separation (tile between burst is
        # negative)
        down_sort_idx, down_min_size, down_mean_min, down_mean_max = oneDGrouping(down_sep)

        # Criterion of bursting
        if down_mean_max > 4 * down_mean_min
            burst_gaps = zeros(Int8, size(down_sep));
            burst_gaps[down_sort_idx[down_min_size+1:end]] .= 1

            up_sort_idx, up_min_size, up_mean_min, up_mean_max = oneDGrouping(up_sep)
            #burst_gaps = logical(burst_gaps);
            
            # Change scope of V for power computation
            first_gap = findfirst(burst_gaps .== 1)
            last_gap = findlast(burst_gaps .== 1)
            # up then down 
            T_start = down_sep_start[first_gap]
            T_end = down_sep_start[last_gap]
            
            # Normalization for power computation
            id_start = findlast(T .<= T_start);
            id_end = findlast(T .<= T_end) + 1;
            V = V[id_start:id_end];
            
            # Same condition as bursting for plateau
            if up_mean_max > 4 * up_mean_min
                plateau = true;
            end

            bursts_cycle = zeros(sum(burst_gaps), 1);
            bursts_size = zeros(sum(burst_gaps), 1);
            bursts_nb_spikes = zeros(sum(burst_gaps), 1);
            bursts_duty = zeros(sum(burst_gaps), 1);
            spike_cycle = zeros(sum(burst_gaps), 1);
            burst_time = zeros(sum(burst_gaps), 1);
            
            # Always start with a down crossing
            if up_crossings[1] < down_crossings[1]
                up_sep = up_sep[2:end];
                up_sep_start = up_sep_start[2:end];
            end
            
            # Finish by an up
            min_length = min(length(up_sep), length(down_sep));
            down_sep = down_sep[1:min_length];
            up_sep = up_sep[1:min_length];
            
            # Start at the up value just after the first large gap
            ind_start = first_gap + 1;

            ind_burst = 0;
            cumulative_time = up_sep[ind_start-1];
            cumulative_spike_cycle = 0;
            nb_spike = 0;
            start_time = 0;
            start_time = up_sep_start[ind_start-1]
            for i = ind_start:min_length
                nb_spike = nb_spike+1;
                # When a new bursting gap is found the cycle is done
                if burst_gaps[i] == 1
                    ind_burst = ind_burst+1;

                    bursts_nb_spikes[ind_burst] = nb_spike;
                    bursts_cycle[ind_burst] = cumulative_time + down_sep[i];
                    bursts_size[ind_burst] = cumulative_time;
                    bursts_duty[ind_burst] = bursts_size[ind_burst]/bursts_cycle[ind_burst];
                    spike_cycle[ind_burst] = cumulative_spike_cycle/(nb_spike-1);
                    burst_time[ind_burst] = start_time;

                    cumulative_time = up_sep[i]
                    start_time = up_sep_start[i]
                    cumulative_spike_cycle = 0;
                    nb_spike = 0;
                else
                    cumulative_time = cumulative_time + down_sep[i] + up_sep[i];
                    cumulative_spike_cycle = cumulative_spike_cycle + down_sep[i] + up_sep[i];
                end
            end

            bursts_cycle = bursts_cycle[1:ind_burst];
            bursts_size = bursts_size[1:ind_burst];
            bursts_nb_spikes = bursts_nb_spikes[1:ind_burst];
            bursts_duty = bursts_duty[1:ind_burst];
            spike_cycle = spike_cycle[1:ind_burst];
            burst_time = burst_time[1:ind_burst]

            nb_spikes = mean(bursts_nb_spikes);
            b_length = mean(bursts_size);
            intra_freq = 1/mean(spike_cycle);
            inter_freq = 1/mean(bursts_cycle);
            duty = mean(bursts_duty);
            
            return (burst_time, bursts_size, b_length, inter_freq, bursts_size)
        # If not bursting then spiking
        else
            return nothing;
        end
    else
        return nothing;
    end
end

# Compute crossings of zero
function computeCrossings(T, V)
    up_crossings = zeros(length(T), 1)
    down_crossings = zeros(length(T), 1)
    ind_upcross = 0
    ind_downcross = 0

    i_last = 1
    prev = V[1]
    for i = 2:lastindex(T)
        curr = V[i]
        if prev < 0 && curr > 0
            ind_upcross = ind_upcross + 1
            # Linear interpolation
            t_prev = T[i_last]
            t_curr = T[i]
            t_inter = (t_prev * curr - t_curr * prev) / (curr - prev)
            up_crossings[ind_upcross] = t_inter
        elseif prev > 0 && curr < 0
            ind_downcross = ind_downcross + 1
            # Linear interpolation
            t_prev = T[i_last]
            t_curr = T[i]
            t_inter = (t_prev * curr - t_curr * prev) / (curr - prev)
            down_crossings[ind_downcross] = t_inter  
        end
        if curr != 0 # If zero continue to see if it crosses and consider the interpolation between the two non zero points the middle
            prev = curr
            i_last = i 
        end
    end
    up_crossings = up_crossings[1:ind_upcross]
    down_crossings = down_crossings[1:ind_downcross]

    return up_crossings, down_crossings
end

# Compute zones of positive and negative value from crossing
function upanddownFromCrossings(up_crossings, down_crossings)
    # Alternance up -> down -> up -> down ... (up -> down) or (up-> down -> up)
    if up_crossings[1] < down_crossings[1]
        down_sep = up_crossings[2:end] - down_crossings[1:length(up_crossings)-1]
        down_sep_start = down_crossings[1:length(up_crossings)-1]
        up_sep = down_crossings[1:end] - up_crossings[1:length(down_crossings)]
        up_sep_start = up_crossings[1:length(down_crossings)]
    # Alternance down -> up -> down ... (down-> up -> down) or (down -> up)
    else
        down_sep = up_crossings[1:end] - down_crossings[1:length(up_crossings)]
        down_sep_start = down_crossings[1:length(up_crossings)]
        up_sep = down_crossings[2:end] - up_crossings[1:length(down_crossings)-1]
        up_sep_start = up_crossings[1:length(down_crossings)-1]
    end

    return down_sep, down_sep_start, up_sep, up_sep_start
end


# Group element of 1D vector into 2 groups based on distance
function oneDGrouping(sep)
    idx = sortperm(sep)
    sep_sort = sep[idx]
    ind_min = 1
    num_min = 1
    sum_min = sep_sort[1]
    ind_max = length(sep_sort)
    sum_max = sep_sort[end]
    num_max = 1
    while ind_min < ind_max - 1
        diff_min = sep_sort[ind_min+1] - sum_min / num_min
        diff_max = sum_max / num_max - sep_sort[ind_max-1]
        if diff_min < diff_max
            ind_min = ind_min + 1
            sum_min = sum_min + sep_sort[ind_min]
            num_min = num_min + 1
        else
            ind_max = ind_max - 1
            sum_max = sum_max + sep_sort[ind_max]
            num_max = num_max + 1
        end
    end

    return idx, ind_min, sum_min/num_min, sum_max/num_max
end