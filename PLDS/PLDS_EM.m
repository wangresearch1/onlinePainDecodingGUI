function [NOWparams newseq Qfun infTime] = PLDS_EM(params,seq, binsize);

% 2016-12-12 
% add output: infTime
% hsl

infTime = 0;

if nargin < 3; 
    binsize = 0.05; % 50 ms
end
 


Q0 = params.model.Q0;    
z0 = params.model.x0;    
 


Trials          = numel(seq); 
maxIter         = params.opts.algorithmic.EMIterations.maxIter;
maxCPUTime      = params.opts.algorithmic.EMIterations.maxCPUTime;
NOWparams       = params;

 

%disp(['Starting EM'])
%disp('----------------------------------------------------------------------------------------------------------------------------')


 
%EMbeginTime = cputime;

sparse_opt = 1;

%%%%%%%%%%%  EM loop
for ii=1:maxIter
tic; 
    NOWparams.state.EMiter = ii;
    infTimeBegin = cputime;
    
    NOWparams.state.EMiter = ii;
    NOWparams.opts.EMiter = ii;
    [newseq Qfun(ii)] = PLDSLaplaceInference(NOWparams,seq);
    %disp(ii);

    %%%%%%% E-step: forward-backward smoother
    %[smooth_state, stateCov_smooth, newseq, Qfun(ii)] = PLDS_filter_smoother(params, seq, z0,Q0, binsize);
    %[smooth_state, stateCov_smooth, newseq, Qfun(ii)] = PLDS_filter_smoother(NOWparams, seq, NOWparams.model.x0,NOWparams.model.Q0, binsize); 
    %[smooth_state, stateCov_smooth, newseq, Qfun(ii)] = PLDS_filter_smoother(NOWparams, seq, z0,Q0, binsize); 
   
%     if (ii>1 && Qfun(ii)<Qfun(ii-1))
%         str = sprintf('ii=%d,q=%f,a=%f,sum(xsm)=%f,sum(Vsm)=%f\n',ii,NOWparams.model.Q,NOWparams.model.A,sum(newseq.posterior.xsm),sum(newseq.posterior.Vsm));
%         disp(str);
%         figure,plot(Qfun);
%         NOWparams = PREVparams;
%         newseq = PREVseq;
%         break;
%     end
%     PREVparams = NOWparams;
%     PREVseq = newseq;
    
    %%%%%%% M-step
    [NOWparams newseq] = PLDSMStep(NOWparams, newseq);
    
    %str = sprintf('q=%f,a=%f,sum(xsm)=%f,sum(Vsm)=%f\n',NOWparams.model.Q,NOWparams.model.A,sum(newseq.posterior.xsm),sum(newseq.posterior.Vsm));
    %disp(str);
%     if sparse_opt == 1      
%         % update state driving noise   
%          tem =  (smooth_state(:,2:end)-smooth_state(:,1:end-1)) * (smooth_state(:,2:end)-smooth_state(:,1:end-1))';
%          Q = sqrtm(tem/(size(smooth_state,2)-1) + 0.00001*eye(size(smooth_state,1))); % L1 norm
%          NOWparams.model.Q = Q;
%     end
    
    %EMTimes(ii) = infTimeEnd-infTimeBegin;
    EMTimes(ii) = toc();
    
    infTime = infTime + EMTimes(ii);
    
    fprintf('\rIteration: %i     Elapsed EM time: %d      Qfun: %d',ii,EMTimes(ii), Qfun(ii))

    
    %if (cputime-EMbeginTime)>maxCPUTime
    if infTime>maxCPUTime
       fprintf('\nReached maxCPUTime for EM, stopping')
       break
    end
end    