function [seq,testseq] = getRecSeq(data,options);
    region = {};
    C = length(data);
    for i=1:C
        region{i} = data(i).region;
    end
    region = unique(region);    
    idx = {};
    for i=1:length(region)
        idxTemp = [];
        for cc=1:C
            if strcmp(data(cc).region,region{i})
                idxTemp = [idxTemp cc];
            end
        end
        idx{i} = idxTemp;
    end    
    seq.region = region;
    
    for i=1:length(seq.region)
        eval(['[seq.' seq.region{i} ',testseq.' seq.region{i} ']= getSeqSpecified(data, idx{' num2str(i) '}, options);']);
    end
end