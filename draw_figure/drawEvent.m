% draw stimulus and withdraw event
% author: sile Hu
% date: 2017-3-16
function lgd = drawEvent(withdrawEvent,options,lgd,trial,subplotIdx)
subplot(subplotIdx);
alignment = options.alignment;
color = eval(['options.plotOpt.' alignment 'Color;']);
p_zero = plot([0 0],[options.plotOpt.ylim],[color '-'],'linewidth',1,'DisplayName',alignment);
lgd = [lgd p_zero];
idx = [];
if ~isempty(withdrawEvent)
idx = find(withdrawEvent(:,1) == trial);
end
if ~isempty(idx)
    if strcmp(alignment,'stimulus')
        name = 'withdrawal';
    else
        name = 'stimulus';
    end
    color = eval(['options.plotOpt.' name 'Color;']);
    pos = withdrawEvent(idx,2)
    p_event = plot([pos pos],[options.plotOpt.ylim],[color '-'],'linewidth',2,'DisplayName',name);
    lgd = [lgd p_event];
end



