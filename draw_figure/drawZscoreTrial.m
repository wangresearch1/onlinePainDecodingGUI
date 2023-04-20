function lgd = drawZscoreTrial(z_score,edges,lgd,decoder,region,options,color,subplotIdx)
subplot(subplotIdx);
decoder = strrep(decoder,'_','\_');
if isfield(z_score,'upperBound')
    if options.plotOpt.shadedCI
        hold on;
        lgd1= fill([edges'; flipdim(edges',1)], [z_score.upperBound'; flipdim(z_score.lowerBound',1)], color,'EdgeColor',[6 6 6]/8,'DisplayName',[region ' ' decoder ' confidence interval']);
        %lgd1= fill([edges'; flipdim(edges',1)], [z_score.upperBound'; flipdim(z_score.lowerBound',1)], 'b','EdgeColor',[6 6 6]/8,'DisplayName',[ ' confidence interval']);
        set(lgd1,'FaceAlpha',0.1);
    else
        hold on;
        lgd1 = plot(edges,z_score.upperBound ,[color '--'],'linewidth',1,'DisplayName',[region ' ' decoder ' confidence interval']);
        hold on;
        plot(edges,z_score.lowerBound ,[color '--'],'linewidth',1);
    end
    if ~options.plotOpt.regionsInOne
    lgd = [lgd lgd1];
    end
end

lgd0 = plot(edges,z_score.zscore ,[color '-'],'linewidth',2,'DisplayName',[region ' ' decoder ' z-score']);
%lgd0 = plot(edges,z_score.zscore ,[color '-'],'linewidth',2,'DisplayName',[' z-score']);
lgd = [lgd lgd0];