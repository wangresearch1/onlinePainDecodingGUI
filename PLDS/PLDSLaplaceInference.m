function [seq varBound] = PLDSLaplaceInference(params,seq)
%
% function seq = PLDSlpinf(params,seq)
%

computeVarBound = false;%true; %!!! make this nice

Trials      = numel(seq);
[yDim xDim] = size(params.model.C);
varBound    = 0;

mps = params.model;
mps.E = [];
mps.D = [];
mps.initV = mps.Q0;
mps.initx = mps.x0;

runinfo.nStateDim = xDim;
runinfo.nObsDim   = yDim;

infparams.initparams = mps;
infparams.runinfo    = runinfo;
infparams.notes      = params.model.notes;

Tmax = max([seq.T]);
for tr=1:Trials
  T = size(seq(tr).y,2);
  indat = seq(tr);
  if isfield(seq(tr),'posterior') && isfield(seq(tr).posterior,'xsm')
    indat.xxInit = seq(tr).posterior.xsm;
  else
    indat.xxInit = getPriorMeanLDS(params,T,'seq',seq(tr));%seq.T=201;seq.y = 12*201 double
    %indat.xxInit = getPriorMeanLDS_sp(params,T,'seq',seq(tr));%seq.T=201;seq.y = 12*201 double
  end
  
  global InferenceCoreTime;%--for analysis
  global InferenceDualCostTime;%--for analysis
  timeInferenceCoreBegin = cputime;%--for analysis
  
  seqRet = PLDSLaplaceInferenceCore(indat, infparams);%indat.T=201;indat.y = 12*201 double;indat.xxInit=1*201 double
  
  timeInferenceCoreEnd = cputime;%--for analysis
  InferenceCoreTime = timeInferenceCoreEnd-timeInferenceCoreBegin;%--for analysis
  
  seq(tr).posterior.xsm      = seqRet.x;
  seq(tr).posterior.Vsm      = reshape(seqRet.V,xDim,xDim*T)';
  seq(tr).posterior.VVsm     = reshape(permute(seqRet.VV(:,:,2:end),[2 1 3]),xDim,xDim*(T-1))';
  seq(tr).posterior.lamOpt   = exp(vec(seqRet.Ypred));
end


if computeVarBound

  VarInfparams    = params.model;
  VarInfparams.CC = zeros(xDim,xDim,yDim);
  for yy=1:yDim%yDim=12
    VarInfparams.CC(:,:,yy) = params.model.C(yy,:)'*params.model.C(yy,:);%CC =1*1*12 double;params.model.C=12*1 double
  end
  VarInfparams.CC = reshape(VarInfparams.CC,xDim^2,yDim);
  
  Cl = {}; for t=1:Tmax; Cl = {Cl{:} params.model.C}; end
  Wmax = sparse(blkdiag(Cl{:}));

  % iterate over trials
  
  for tr = 1:Trials

    T = size(seq(tr).y,2);
    VarInfparams.d = repmat(params.model.d,T,1); if params.model.notes.useS; VarInfparams.d = VarInfparams.d + vec(seq(tr).s); end
    VarInfparams.mu         = vec(getPriorMeanLDS(params,T,'seq',seq(tr)));
    VarInfparams.W          = Wmax(1:yDim*T,1:xDim*T);
    VarInfparams.y          = seq(tr).y;
    VarInfparams.Lambda     = buildPriorPrecisionMatrixFromLDS(params,T);
    VarInfparams.WlamW      = sparse(zeros(xDim*T));
    VarInfparams.dualParams = [];
  
    if isfield(params.model,'baseMeasureHandle')
      VarInfparams.DataBaseMeasure = feval(params.model.baseMeasureHandle,seq(tr).y,params);
      seq(tr).posterior.DataBaseMeasure = VarInfparams.DataBaseMeasure;
    end

    lamOpt = seq(tr).posterior.lamOpt;
    
    timeInferenceDualCostBegin = cputime;%--for analysis
    
    [DualCost, ~, varBound] = VariationalInferenceDualCost(lamOpt,VarInfparams);
    
    timeInferenceDualCostEnd = cputime;%--for analysis
    InferenceDualCostTime = timeInferenceDualCostEnd-timeInferenceDualCostBegin;%--for analysis
    
    seq(tr).posterior.varBound = varBound;

  end
  varBound = 0;
  for tr=1:Trials; varBound = varBound + seq(tr).posterior.varBound; end; 
end

binsize = 0.05;
for tr = 1:Trials
y = seq(tr).y; 
A = params.model.A; % m-by-m
C = params.model.C; % C-by-m
d = params.model.d; % C-by-1
Q = params.model.Q; % m-by-m
smooth_state = seqRet.x;
stateCov_smooth = seqRet.V;
% correlation estimation (Smith & Brown, 2003)
QQ1 = zeros(1,T);
QQ2 = zeros(1,T);

for t=1:T
    QQ1(t) = stateCov_smooth(t) + smooth_state(t) * smooth_state(t)';% Wk
end
for t=1:T-1
    tem = A * stateCov_smooth(t+1);
    QQ2(t) = tem + smooth_state(t) * smooth_state(t+1)';% Wk,k+1
end

% compute the Q function
Qfun = 0;
% compute E[log p(N0,t|x,theta*)]
for t=1:T
    rate = exp(C * smooth_state(:,t) + d);
    tem1 = sum(y(:,t) .* (C * smooth_state(:,t) + d + log(binsize)) - rate * binsize);   % sum wrt channel
    Qfun = Qfun + tem1; % sum wrt time
end    


%NOTE:  a'*b*a = trace(a*a'*b) 

% noise covariance Q has to be full rank
% compute E[log p(x|Rho,alpha,sigma^2)] neglecting last two terms
for t=1:T-1
    tem2 = -0.5 * trace( (QQ1(t+1) - 2*A*QQ2(t) + A*QQ1(t)*A') * inv(Q));  
    Qfun = Qfun + tem2 - 0.5*log(det(Q)) -0.5*log(2*pi);
end
seq(tr).posterior.varBound = Qfun;
varBound = Qfun;
end


