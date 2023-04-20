
function model = trainModelSingleSeqPLDS(seq,options)

%ignore all zero units
noSpike = [];
for i=1:size(seq(1).y,1)
    if sum(seq(1).y(i,:))<1
        noSpike = [noSpike i];
    end
end
for trial = 1:length(seq)
    seq(trial).y(noSpike,:) = [];
    % initialize
    params = [];
    if isempty(seq(trial).y)
        continue;
    end
    paramsInit = PLDSInitialize(seq(trial),options.xdim,options.initMethod,params); % xDim = 1
    
    % EM iteration - model training
    paramsInit.opts.algorithmic.EMIterations.maxIter     = 200; %50
    paramsInit.opts.algorithmic.EMIterations.maxCPUTime  = 300; %unit: s    
    if trial>=300
       [params newseq varBound infTime] = PLDS_EM(paramsInit,seq(trial-2:trial),options.binsize);
    else
       [params newseq varBound infTime] = PLDS_EM(paramsInit,seq(trial),options.binsize);
    end
    
    % set the C and d of that unit with arbitrary value
    noSpikeC = min(abs(params.model.C))/100;
    noSpiked = mean(params.model.d);
    C = params.model.C;
    d = params.model.d;
    for i=1:length(noSpike)
        C = [C(1:noSpike(i)-1,:);noSpikeC;C(noSpike(i):end,:)];
        d = [d(1:noSpike(i)-1);noSpiked;d(noSpike(i):end)];
    end
    params.model.C = C;
    params.model.d = d;
    
    model(trial).params = params;
    model(trial).newseq = newseq;
    model(trial).varBound = varBound;
    model(trial).infTime = infTime;
end