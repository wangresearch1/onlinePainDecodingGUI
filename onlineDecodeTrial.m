% online decoding based on trained model, trial structure
% author: Sile Hu
% date: 2017-3-13
function onlineDecodeResult = onlineDecodeTrial(seq,testseq,model,options)
for i=1:length(options.decoders)
    decoder = options.decoders{i};
    decodeResult = eval(['getDecodeResultTrial' decoder '(seq,testseq,model,options);']);
    eval(['onlineDecodeResult.' decoder ' = decodeResult;']);
end