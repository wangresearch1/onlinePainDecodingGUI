function [C, X, d] = ExpFamPCA(Y,xDim,varargin)
%
% [C, X, d] = ExpFamPCA(Y,xDim)
%
% exponential family PCA, currently only implemented for
% exponential-Poisson observation model, i.e. learns C, X and d for model 
% Y ~ Poisson(exp(Cx+d+s));
%
% inputs:
% Y:     matrix of size yDim x T
% s:     additive , observed input, same size as Y or scalar (optional)
% xDim:  scalar, dimensionality of latent space
%
% output: 
% C:      loading matrix, of size yDim x xDim
% X:      recovered latent factors, of size xDim x T
% d:      mean offset
%
%
% (c) Lars Buesing 2014


s                   = 0;
dt                  = 10;    % rebin factor %!!! this is very much dependent on the firing rates
lam                 = 1e-1;  % penalizer
CposConstrain       = false; % constrain C to be have pos elements

options.display     = 'none';
options.MaxIter     = 10000;
options.maxFunEvals = 50000;
options.Method      = 'lbfgs';
options.progTol     = 1e-9;
options.optTol      = 1e-5;

assignopts(who,varargin);


seqDum.y = Y;
seqDum = rebinRaster(seqDum,dt);
Y = seqDum.y;

if numel(s)>1.5; s = subsampleSignal(s,dt);end

[yDim T] = size(Y);

%% rough initialization for ExpFamPCA based on SVD
my = mean(Y-s,2);
[Uy Sy Vy] = svd(bsxfun(@minus,Y-s,my),0);
my = max(my,0.1);

Cinit = Uy(:,1:xDim);
if CposConstrain
  Cinit = 0.1*randn(yDim,xDim);
end

%Xinit = [0.0101868528212858,-0.00133217479507735,-0.00714530163787158,0.0135138576842666,-0.00224771056052584,-0.00589029030720801,-0.00293753597735416,-0.00847926243637934,-0.0112012830124373,0.0252599969211831,0.0165549759288735,0.00307535159238252,-0.0125711835935205,-0.00865468030554804,-0.00176534114231451,0.00791416061628634,-0.0133200442131525,-0.0232986715580508,-0.0144909729283874,0.00333510833065806,0.00391353604432901,0.00451679418928238,-0.00130284653145721,0.00183689095861942,-0.00476153016619074,0.00862021611556922,-0.0136169447087075,0.00455029556444334,-0.00848709379933659,-0.00334886938964048,0.00552783345944550,0.0103909065350496,-0.0111763868326521,0.0126065870912090,0.00660143141046978,-0.000678655535426873,-0.00195221197898754,-0.00217606350143192,-0.00303107621351741,0.000230456244251053];
Xinit = 0.01*randn(xDim,T);
dinit = log(my);
% CXdinit = [vec([Cinit; Xinit']); dinit];
CXdinit = [vec([Cinit; Xinit'])];

%run ExpFamCPA  
%CXdOpt  = minFunc(@ExpFamPCACost,CXdinit,options,Y,xDim,lam,s,CposConstrain); 
CXdOpt  = minFunc(@ExpFamPCACost,CXdinit,options,Y,xDim,lam,s,CposConstrain,dinit); 

% Function returns all parameters lumped together as one vector, so need to disentangle: 
% d  = CXdOpt(end-yDim+1:end);
% CX = reshape(CXdOpt(1:end-yDim),yDim+T,xDim);
d = dinit;
CX = reshape(CXdOpt(1:end),yDim+T,xDim);
C  = CX(1:yDim,:);
if CposConstrain; C = exp(C); end
X  = CX(yDim+1:end,:)';
d  = d-log(dt);
Xm = mean(X,2);
X  = bsxfun(@minus,X,Xm);
d  = d+C*Xm;

if ~CposConstrain
  % transform C to have orthonormal columns
  [UC SC VC] = svd(C);
  M = SC(1:xDim,1:xDim)*VC(:,1:xDim)';
  C = C/M;
  X = M*X;

  % transform for X to have orthogonal rows
  [MU MS MV] = svd((X*X')./T);
  M = MU';
  C = C/M;
  X = M*X;
end