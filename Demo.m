%% --- add toolbox path ---
clear;clc;
plds_core_path = './PLDS';
addpath(genpath(plds_core_path));
minFunc_path = './minFunc';
addpath(genpath(minFunc_path));
data_path = '';
addpath(genpath(data_path));
draw_path = './draw_figure/'; %path of draw figure functions
addpath(draw_path);
clear;clc;
%% --- set options ---
loadModel = 0;
saveModel = 1;
% directory for saving trained models
model_dir = './model';
dataOpt = 'Spikedata';
data = load([dataOpt '.mat']);

options.binsize = 0.1; %unit: s
options.TPre = 5;       %unit: s
options.TPost = 5;     %unit: s
options.alignment = 'stimulus';
options.initMethod = 'ExpFamPCA';%'NucNormMin';%'ExpFamPCA';
options.xdim = 1;

% get trial sequence
[seq,testseq] = getRecSeq(data.Spikedata,options);
%% --- train model ---
if loadModel
    filename = [model_dir '/' dataOpt '.mat'];  
    load(filename);
    model = eval([dataOpt '.model']);
    options = eval([dataOpt '.options']);
    options.region = seq.region;
else
    for i=1:length(seq.region)
       eval(['model.' seq.region{i} '=trainModelSingleSeqPLDS(seq.' seq.region{i} ',options )']);
    end
    if saveModel
        filename = [model_dir '/' dataOpt '.mat'];  
        eval([dataOpt '.model=model;']);
        eval([dataOpt '.options=options;']);
        save(filename, dataOpt);
    end
    options.region = seq.region;
end
%% --- Decoding ---
options.decoders = {'PLDS_forward'}; % decode method
options.baseline = [1 4]; % [start end] unit:s, 0 is the start of seq;
options.currentTrial = 0; % Use current model or previous model, 0: Previous / 1: Current

% --- compute z-score ---
options.confidenceInterval = 2; % confidence interval = options.confidenceInterval * standard deviation
options.threshold = 1.65; % 0.95 confidence threshold
options.rou = [0.2,0.4,0.6,0.8]; % forgetting factor for CCF

% 1) decode evoked pain 
options.trialType=1; % 1: evoked pain trial / 0: spontaneous pain trial 
onlineDecodeResult1 = onlineDecodeTrial(seq,testseq,model,options);
z_scores1 = computePvalueTrial(onlineDecodeResult1,seq,options);


%% draw figures
plotOpt.region ={'ACC'};
plotOpt.trials = {'all'};%[1 2];
plotOpt.baseline = 1;
plotOpt.legend = 1;
plotOpt.withdraw = 1;
plotOpt.zscoreColor = ['r','b','m','c','y','k'];
plotOpt.baseColor = 'b';
plotOpt.stimulusColor = 'g';
plotOpt.withdrawalColor = 'k';
plotOpt.threshColor = 'k';
plotOpt.shadedCI = 1; % whether the confidence interval is drew as shaded area or not
plotOpt.ylim = [-8 8];
plotOpt.dirSaveFigure = dataOpt;
plotOpt.regionsInOne = 1;
options.plotOpt = plotOpt;

% if options.trial = -1, plot all trials
options.trial = -1;
drawZscorePlots_Correlation(z_scores1,seq,options);

