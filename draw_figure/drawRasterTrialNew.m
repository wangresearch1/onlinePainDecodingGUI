% draw raster for a trial on the plot handled by h
% author: Sile Hu
% date: 2017-3-13
% input: seq - spike sequence
%        sub   - subplot index str
function drawRasterTrialNew(seq,titleStr,edges,sub)
% -- raster --
subplot(sub);
imagesc(edges, [1:size(seq.y,1)],seq.y);axis xy
colormap(flipud(bone));
% for i=1:size(seq.y,1)
%     find(seq.y(i,:))
%     line([temp'; temp'],[(trial-1)*ones(size(temp')); (trial)*ones(size(temp'))], 'linewidth',2,'color','k');
% end
ylabel('Unit','fontsize',24);
%xlabel('Time(s)','fontsize',24);
set(gca,'FontSize',20);
title(titleStr,'fontsize',20);