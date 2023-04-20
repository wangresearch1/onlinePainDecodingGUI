function [seq,testseq] = getSeqSpecified(data, idx, options)
    
    binsize = options.binsize;
    TPre = options.TPre;
    TPost = options.TPost;

    edges = [-TPre:binsize:TPost];
    NoTrials = length(data(1).evoked_onset);
    seq = struct('T', length(edges), 'y',zeros(length(idx),length(edges)));
    for trial= 1:NoTrials
        Spikecount = zeros(length(idx),length(edges));
        trigger = data(idx(1)).evoked_onset; % stimulus
        for cc=1:length(idx)            
            ind = find(data(idx(cc)).spikes>=trigger(trial)-TPre & data(idx(cc)).spikes<=trigger(trial)+TPost);
            count = histc(data(idx(cc)).spikes(ind)-trigger(trial), edges);
            Spikecount(cc,:) = count;
        end
        seq = setfield(seq,{trial},'T', size(Spikecount,2));
        seq = setfield(seq,{trial},'y', Spikecount);
    end
    
    testseq = struct();
    for trial= 1:NoTrials         
        testtrigger = data(idx(cc)).spon_onset; 
        if trial~=NoTrials
            edges = [binsize:binsize:trigger(trial+1)-trigger(trial)];
            testind= testtrigger>trigger(trial) & testtrigger<trigger(trial+1);
        else
            edges = [binsize:binsize:max(trigger(NoTrials),testtrigger(end))+TPost-trigger(NoTrials)];
            testind= testtrigger>trigger(NoTrials);
        end
        Spikecount = zeros(length(idx),length(edges));
        mark = floor((testtrigger(testind)-trigger(trial))/binsize)+1;
        for cc=1:length(idx)
            if trial~=NoTrials
                ind = find(data(idx(cc)).spikes>trigger(trial) & data(idx(cc)).spikes<=trigger(trial+1));
            else
                ind = find(data(idx(cc)).spikes>trigger(NoTrials) & data(idx(cc)).spikes<=max(trigger(NoTrials),testtrigger(end)));
            end
            count = histc(data(idx(cc)).spikes(ind)-trigger(trial), edges);
            Spikecount(cc,:) = count;
        end
        testseq = setfield(testseq,{trial},'T', size(Spikecount,2));
        testseq = setfield(testseq,{trial},'y', Spikecount);
        testseq = setfield(testseq,{trial},'Test', mark);
    end
end