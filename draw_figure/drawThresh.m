% draw pain detection threshold event
% author: sile Hu
% date: 2017-3-16
function lgd = drawThresh(options,edges,lgd,subplotIdx)
if ~strcmp(subplotIdx, '');
    subplot(subplotIdx);
end
hold on;
p_thr165 = plot([edges(1) edges(end)],[options.threshold options.threshold],[options.plotOpt.threshColor '--'],'linewidth',1,'DisplayName',['z-score threshold:' num2str(options.threshold)]);
hold on;
plot([edges(1) edges(end)],[-options.threshold -options.threshold],[options.plotOpt.threshColor '--'],'linewidth',1);
lgd = [lgd p_thr165];