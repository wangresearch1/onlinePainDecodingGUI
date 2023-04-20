function params = LDSMStepLDS_sp(params,seq)
%
% function params = MStepLDS(params,seq)
%
% Parameters to update: A,Q,Q0,x0,B
%
%
% (c) L Buesing 2014
%
% simplified version of LDSMStepLDS, for a specific case of use.
%
% update A,Q,x0,Q0 by LDS closed form solution
%
% remove useB branches
% remove isfield(posterior.Vsm) branches
% remove for tr = 1:trials loop because trials = 1;
% redefine variable dimensions for scalar x;
% remove uDim,xDim
% Sile Hu 2016-11-21
%

%% compute posterior statistics
T = size(seq.y,2);

Vsm   = seq.posterior.Vsm';
VVsm  = seq.posterior.VVsm';

MUsm0 = seq.posterior.xsm(1:T-1);%pre term
MUsm1 = seq.posterior.xsm(2:T);  %latter term 

S11 = sum(Vsm(2:T))    + MUsm1*MUsm1';% sum(postCov(2:T))+sum(x^2(2:T))
S01 = sum(VVsm(1:T-1)) + MUsm0*MUsm1';% sum(postCovk;k-1(1:T-1))+sum(xk*xk-1(2:T))
S00 = sum(Vsm(1:T-1))  + MUsm0*MUsm0';% sum(postCov(1:T-1))+sum(x^2(1:T-1))

params.model.A  = S01'/S00;
params.model.Q  = (S11+params.model.A*S00*params.model.A - 2*S01*params.model.A)./(T-1);%(S11+A^2*S00-2*S01*A)/(T-1)

%str = sprintf('q=%f,a=%f,S11=%f,S00=%f,S01=%f\n',params.model.Q,params.model.A,S11,S00,S01);
%disp(str);

params.model.x0 = MUsm0(1);
params.model.Q0 = Vsm(1);

%for debugging
if (min(eig(params.model.Q))<0) || (min(eig(params.model.Q0))<0)
   keyboard
   params.model.Q=params.model.Q+1e-9;
end