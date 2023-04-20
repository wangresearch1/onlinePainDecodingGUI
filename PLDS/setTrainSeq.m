function TrainSeqOut = setTrainSeq(TrainSeq,TrainTrialOpt,NoTrials)
% training trial option (D)
% 1.'CurTrial' - current trial* now only consider current trial
% 2.'1stTrial' - first trial
% 3.'preTrial' - previous trial
switch TrainTrialOpt
    case 'CurTrial' % 1.current trial
        TrainSeqOut = TrainSeq;
    case '1stTrial' % 2.first trial
        TrainSeqOut = repmat(TrainSeq(1),1,NoTrials);
    case 'preTrial' % 3.previous trial
        TrainSeqOut = [TrainSeq(1),TrainSeq(1:NoTrials-1)];
    otherwise
        TrainSeqOut = TrainSeq;
end