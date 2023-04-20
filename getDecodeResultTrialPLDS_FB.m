% get forward backward PLDS decoding result(the result get while training)
% author: Sile Hu
% date: 2017-3-13
% 2017-08-07 add single trial option, Sile Hu
function decodeResult = getDecodeResultTrialPLDS_FB(seq,model,options)
for j=1:length(seq.stimulus)
    trials = eval(['seq.ntrial' seq.stimulus{j}]);
    st = 1;
    et = trials;
    if isfield(options,['trial' seq.stimulus{j}])
        single_trial = eval(['options.trial' seq.stimulus{j}]);
        if (single_trial>=1 && single_trial<=trials)
            st = single_trial;
            et = single_trial;
        end
    end
    for i=1:length(seq.region)
       for k=st:et
           eval(['decodeResult.' seq.region{i} seq.stimulus{j} '(' num2str(k) ').x = model.' seq.region{i} seq.stimulus{j} '('  num2str(k) ').newseq.posterior.xsm;']);
           eval(['decodeResult.' seq.region{i} seq.stimulus{j} '(' num2str(k) ').V = model.' seq.region{i} seq.stimulus{j} '('  num2str(k) ').newseq.posterior.Vsm'';']);
       end
    end
end