function params = PLDSInitialize(seq,xDim,initMethod,doubleRegion,params)
%
% function params = PLDSInitialize(params,seq)
%
% inititalize parameters of Population-LDS model with different methods. At
% the moment, focusses on exponential link function and Poisson
% observations.
%
%input:
% seq:      standard data struct
% xdim:     desired latent dimensionality
% initMethods:
%
% - params											% just initialize minimal undefiened fields with standard values
% - PLDSID											% moment conversion + Ho-Kalman SSID
% - ExpFamPCA									    % exponential family PCA
% - NucNormMin										% nuclear norm minimization, see [Robust learning of low-dimensional dynamics from large neural ensembles David Pfau, Eftychios A. Pnevmatikakis, Liam Paninski. NIPS2013]
% params: if initialization method 'params' is chosen, the params-struct
% that one should use
%
% (c) L Buesing 2014


yDim       = size(seq(1).y,1);
if(doubleRegion==1)
    nACC = seq(1).nACC;
else
    nACC = [];
end
Trials     = numel(seq);
params.opts.initMethod = initMethod;
params.opts.doubleRegion = doubleRegion;
params     = PLDSsetDefaultParameters(params,xDim,yDim,nACC);					% set standard parameter values


switch initMethod
    
    case 'params'
        % do nothing
        disp('Initializing PLDS parameters with given parameters')
        
        
    case 'PLDSID'
        % !!! debugg SSID stuff separately & change to params.model convention
        disp('Initializing PLDS parameters using PLDSID')
        if params.model.notes.useB
            warning('SSID initialization with external input: not implemented yet!!!')
        end
        PLDSIDoptions = struct2arglist(params.opts.algorithmic.PLDSID);
        params.model = FitPLDSParamsSSID(seq,xDim,'params',params.model,PLDSIDoptions{:});
        
    case 'ExpFamPCA'
        % this replaces the FA initializer from the previous verions...
        disp('Initializing PLDS parameters using exponential family PCA')
        
        dt = params.opts.algorithmic.ExpFamPCA.dt;
        Y  = [seq.y];
        if params.model.notes.useS; s = [seq.s];
        else s=0;end
        [Cpca, Xpca, dpca] = ExpFamPCA(Y,xDim,'dt',dt,'lam',params.opts.algorithmic.ExpFamPCA.lam,'options',params.opts.algorithmic.ExpFamPCA.options,'s',s);
        params.model.Xpca = Xpca;
        params.model.C = Cpca;
        params.model.d = dpca;
        
        if params.model.notes.useB; u = [seq.u];else;u = [];end
        params.model = LDSObservedEstimation(Xpca,params.model,dt,u);
        
        
    case 'NucNormMin'
        disp('Initializing PLDS parameters using Nuclear Norm Minimization')
        
        dt = params.opts.algorithmic.NucNormMin.dt;
        seqRebin.y = [seq.y]; seqRebin = rebinRaster(seqRebin,dt);
        Y  = [seqRebin.y];
        options = params.opts.algorithmic.NucNormMin.options;
        options.lambda = options.lambda*sqrt(size(Y,1)*size(Y,2));
        if params.model.notes.useS
            Yext = subsampleSignal([seq.s],dt);
        else
            Yext = [];
        end
        [Y,Xu,Xs,Xv,d] = MODnucnrmminWithd( Y, options , 'Yext', Yext );
        params.model.d = d-log(dt);
        
        if ~params.opts.algorithmic.NucNormMin.fixedxDim
            disp('Variable dimension; still to implement!')
        else
            %params.model.C = factoran(log(seqRebin.y'+1)/0.05,1);%Xu(:,1:xDim)*Xs(1:xDim,1:xDim);
            params.model.C = Xu(:,1:xDim)*Xs(1:xDim,1:xDim);
            %params.model.C = ffa(log(seqRebin.y'+1)/0.05,1);
            if params.model.notes.useB; u = [seq.u];else;u = [];end
            params.model = LDSObservedEstimation(Xv(:,1:xDim)',params.model,dt,u);
            params.model.Xpca = Xv(:,1:xDim)';
            params.model.Xs   = diag(Xs(1:xDim,1:xDim));
        end
        
        
    otherwise
        warning('Unknown PLDS initialization method')
        
end

if params.model.notes.useB && (numel(params.model.B)<1)
    params.model.B = zeros(xDim,size(seq(1).u,1));
end

params.modelInit = params.model;
