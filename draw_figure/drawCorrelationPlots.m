% draw basline area
% author: sile Hu
% date: 2017-3-16
function lgd = drawCorrelationPlots(cct,options,edges,lgd,subplotIdx,color,rou)
if ~strcmp(subplotIdx, '')
    subplot(subplotIdx);
end
hold on;
p_base = plot(edges,cct,[color '-'],'linewidth',2,'DisplayName',['rho = ' num2str(rou)]);
if(rou==0.6)
    p_thresh1 = plot([edges(2) edges(end)],[options.C_thresh(1) options.C_thresh(1)],[options.plotOpt.threshColor '--'],'linewidth',1,'DisplayName',['cross-value threshold:' num2str(options.C_thresh(1))]);
    p_thresh2 = plot([edges(2) edges(end)],[options.C_thresh(2) options.C_thresh(2)],[options.plotOpt.threshColor '--'],'linewidth',1,'DisplayName',['cross-value threshold:' num2str(options.C_thresh(2))]);
%     p_base = plot([options.baseline(1)-options.TPre options.baseline(2)-options.TPre],[0 0],'b-','linewidth',100,'DisplayName','baseline');
%     range = [options.baseline(1)-options.TPre options.baseline(2)-options.TPre];
    lgd = [lgd p_base];% p_base p_thresh1 p_thresh2];
else    
    lgd = [lgd p_base];% p_base];% p_thresh1 p_thresh2];
end