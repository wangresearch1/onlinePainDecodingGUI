% draw basline area
% author: sile Hu
% date: 2017-3-16
function lgd = drawBaseline(options,edges,lgd,subplotIdx)
if ~strcmp(subplotIdx, '');
    subplot(subplotIdx);
end

%p_base = plot([options.baseline(1)-options.TPre options.baseline(2)-options.TPre],[0 0],'-','Color',[0.4 0.6 0.7] ,'linewidth',20,'DisplayName','baseline');
%p_base = plot([options.baseline(1)-options.TPre options.baseline(2)-options.TPre],[0 0],'b-','linewidth',100,'DisplayName','baseline');
range = [options.baseline(1)-options.TPre options.baseline(2)-options.TPre];
p_base = fill([range'; flipdim(range',1)], [ones(length(range))*options.plotOpt.ylim(1); flipdim(ones(length(range))*options.plotOpt.ylim(2),1)], [6 6 6]/8,'EdgeColor',[6 6 6]/8,'DisplayName','baseline');
%p_base = fill([edges'; flipdim(edges',1)], [ones(length(edges))*10; flipdim(-ones(length(edges))*10,1)], [6 6 6]/8,'EdgeColor',[6 6 6]/8,'DisplayName','baseline');
lgd = [lgd p_base(1)];