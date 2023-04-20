% get forward backward PLDS decoding result(the result get while training)
% author: Sile Hu, yuehusile@gmail.com
% date: 2017-3-13
% 2017-08-07 add single trial option
function decodeResult = getDecodeResultTrialPLDS_forward(seq,testseq,model,options)

    trials = length(eval(['seq.' options.region{1}]));
    st = 1;
    et = trials;

    for i=1:length(seq.region)
        numtest = 1;  % index of test trials
        for k=st:et
            params = eval(['model.' seq.region{i} '('  num2str(k) ').params;']);
            if (options.currentTrial)
                ytest = eval(['seq.' seq.region{i} '('  num2str(k) ').y;']);
                last_z0 = eval(['model.' seq.region{i} '('  num2str(k) ').newseq(end).posterior.xsm(:,1);']);
                if(size(last_z0,1)==2)
                    last_Q0 = eval(['model.' seq.region{i} '('  num2str(k) ').newseq(end).posterior.Vsm(1:2,:);']);
                else
                    last_Q0 = eval(['model.' seq.region{i} '('  num2str(k) ').newseq(end).posterior.Vsm(1,:);']);
                end
            else
                if  ~options.trialType 
                    kk=k;
                    ytest = eval(['testseq.' seq.region{i} '('  num2str(kk) ').y;']);              
                    params = eval(['model.' seq.region{i}  '('  num2str(kk) ').params;']);
                    last_z0 = eval(['model.' seq.region{i} '('  num2str(kk) ').newseq(end).posterior.xsm(:,end);']);
                    last_Q0 = eval(['model.' seq.region{i} '('  num2str(kk) ').newseq(end).posterior.Vsm(end,:);']);
                else
                    kk=k;
                    ytest = eval(['seq.' seq.region{i}  '('  num2str(k) ').y;']);
                    if kk>1,kk=k-1;else,kk=et;end                    
                    params = eval(['model.' seq.region{i} '('  num2str(kk) ').params;']);
                    last_z0 = eval(['model.' seq.region{i} '('  num2str(kk) ').newseq(end).posterior.xsm(:,1);']);
                    last_Q0 = eval(['model.' seq.region{i} '('  num2str(kk) ').newseq(end).posterior.Vsm(1,:);']);
                end
            end
            [online_state, online_var] = PLDS_online_filtering(params,ytest,last_z0,last_Q0,options.binsize);
            if ~options.trialType && ~options.currentTrial && isfield(eval(['testseq.' seq.region{i} '('  num2str(k) '),']), 'Test')
                test = eval(['testseq.' seq.region{i} '('  num2str(k) ').Test;']);              
                if ~isempty(test)
                    for ii=1:length(test)
                        edges=[test(ii)-options.TPre/options.binsize:test(ii)+options.TPost/options.binsize];
                        if(edges(1)<=0),edges = 1;end
                        eval(['decodeResult.' seq.region{i} '(' num2str(numtest) ').x =online_state(:,edges);']);
                        eval(['decodeResult.' seq.region{i} '(' num2str(numtest) ').V =online_var(:,edges);']);
                        numtest = numtest + 1;
                    end
                else
                    eval(['decodeResult.' seq.region{i} '(' num2str(numtest) ').x =[];']);
                    eval(['decodeResult.' seq.region{i} '(' num2str(numtest) ').V =[];']);
                end
            else
                eval(['decodeResult.' seq.region{i} '(' num2str(k) ').x =online_state;']);
                eval(['decodeResult.' seq.region{i} '(' num2str(k) ').V =online_var;']);
            end
        end
    end
end