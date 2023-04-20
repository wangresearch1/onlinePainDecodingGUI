
function drawZscorePlots_Correlation(z_scores,seq,options)

    decoders = options.decoders;
    if strcmp(options.plotOpt.region,'all')
        region = seq.region;
    else
        region = options.plotOpt.region;
    end

    legends = {};
    edges = [-options.TPre:options.binsize:options.TPost];

    if strcmp(options.plotOpt.trials,'all')
        trials = (1:length(eval(['z_scores.' decoders{1} '.' options.region{1}]))-1);
    else
        trials = options.plotOpt.trials;
    end
    st = 1;
    et = length(trials);
    single_trial = options.trial;
    if (single_trial>=1 && single_trial<=et)
        trials = single_trial;
        et = 1;
    end
  
    for t=st:et
        figure('name',[' trial' num2str(trials(t))]);
        subplotIdx = '211';
        seqall = eval(['seq.' seq.region{1} '(trials(t))']);
        titleStr = ['Trial' num2str(trials(t)) ' ' region{1} '=1:' num2str(size(seqall.y,1))];        
        if(length(seq.region)==1)
            seqall.y = [seqall.y;eval(['seq.' seq.region{1} '(trials(t)).y'])];
        else
            seqall.y = [seqall.y;eval(['seq.' seq.region{2}  '(trials(t)).y'])];
        end
        
        drawRasterTrialNew(seqall,titleStr,edges,subplotIdx);          
        
        % lower plot
        subplotIdx = '212';
        % draw baseline
        legends = drawBaseline(options,edges,legends,subplotIdx);
        legends = drawThresh(options,edges,legends,subplotIdx);

        for r=1:length(region)
            if (r==2)                                
                if (length(seq.region)==1)
                    z_score = eval(['z_scores.' decoders{1} '.' seq.region{1} '(trials(t));']);
                else
                    z_score = eval(['z_scores.' decoders{1} '.' seq.region{2} '(trials(t));']);
                end
                l_z = length(z_score.zscore) - 1;
                decoder_edges = [-options.TPre:options.binsize:l_z*options.binsize - options.TPre];
                if options.currentTrial
                    decoder_edges = decoder_edges + options.binsize;
                end
                color = options.plotOpt.zscoreColor(d+r-1);
                legends = drawZscoreTrial(z_score,decoder_edges,legends,decoders{d},region{r},options,color,subplotIdx);                           
            else
                for d=1:length(decoders)
                    z_score = eval(['z_scores.' decoders{1} '.' seq.region{1} '(trials(t));']);
                    l_z = length(z_score.zscore) - 1;
                    decoder_edges = [-options.TPre:options.binsize:l_z*options.binsize - options.TPre];
                    if options.currentTrial
                        decoder_edges = decoder_edges + options.binsize;
                    end
                    color = options.plotOpt.zscoreColor(d+r-1);
                    legends = drawZscoreTrial(z_score,decoder_edges,legends,decoders{d},region{r},options,color,subplotIdx);
                end
            end
        end

        % draw event
        if isfield(seq,'withdrawal')
            withdrawEvent = seq.withdrawal;
        else
            withdrawEvent = [];
        end
        legends = drawEvent(withdrawEvent,options,legends,trials(t),subplotIdx);

        ylabel('Z-score','fontsize',24);
        set(gca,'fontsize',20);
        ylim(options.plotOpt.ylim);
%         xlabel('time(s)','fontsize',24);
        xlim([-options.TPre,options.TPost])
        subplot(subplotIdx)
%         lgd = legend(legends);
%         lgd.FontSize = 10;
        legends = [];


    end
end