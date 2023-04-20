
% compute z-score and CCF value
% author: Zhengdong Xiao, zhengdong914@gmail.com

function z_scores= computePvalueTrial(onlineDecodeResult,seq,options)

    baselineRange = [(options.baseline(1)/options.binsize): (options.baseline(2)/options.binsize)];
    trials = length(eval(['onlineDecodeResult.' options.decoders{1} '.' options.region{1}]));
    st = 1;
    et = trials;

    for i=1:length(seq.region)
        if(i==2)
           options.confidenceInterval=2;
        else
           options.confidenceInterval=2;    % for ACC could be 2
        end
        
        for k=st:et
            x = eval(['onlineDecodeResult.' options.decoders{1} '.' seq.region{i} '('  num2str(k) ').x']);
            V = eval(['onlineDecodeResult.' options.decoders{1} '.' seq.region{i} '('  num2str(k) ').V']);
            x_error=zeros(size(x));
            if ~isempty(V) && length(V)>1
                if options.trialType
                    mean_base = mean(x(:,baselineRange),2);
                else
                    tmp = eval(['onlineDecodeResult.' options.decoders{1} '.' seq.region{i} '('  num2str(k) ').x']);
                    mean_base = mean(tmp(:,baselineRange),2);
                end
                mean_x = mean(x,2);
                if(size(x,1)==2)
                    if options,trialType
                        std_base = cov(x(:,baselineRange)');
                    else
                        std_base = cov(tmp(:,baselineRange)');
                    end
                    for m=1:size(x,2)
                        Q=V(:,m*(1:2));
                        [vec,val]=eig(Q);
                        vec=real(vec);
                        if(val(1,1)>val(2,2))
                            Axis=[sqrt(val(1,1)*5.991) sqrt(val(2,2)*5.991)];
                            angle=atan2(vec(2,1),vec(1,1));
                        else
                            Axis=[sqrt(val(2,2)*5.991) sqrt(val(1,1)*5.991)];
                            angle=atan2(vec(2,2),vec(1,2));
                        end
                        ParG=[x(:,m)',Axis,angle]';
                        XY=mean_base';
                        [RSS, XYproj] = Residuals_ellipse(XY,ParG);
                        x_error(:,m)=real(XYproj');
                    end
                    if(mean_x(1,1)>mean_base(1,1)&&mean_x(2,1)>mean_base(2,1))
                        pValue = 1-mvncdf(x_error',mean_base',std_base);
                    else
                        pValue = mvncdf(x_error',mean_base',std_base);
                    end
                    eval(['z_scores.' options.decoders{d} '.' seq.region{i} seq.stimulus{j} '(' num2str(k) ').zscore = x']);
                else
                   if options.trialType
                       std_base = std(x(baselineRange));
                   else
                       std_base = std(tmp(baselineRange));
                   end
                   for m=1:size(x,2)
                       if(x(:,m)< mean_base)
                           x_error(:,m)=x(:,m) + options.confidenceInterval * sqrt(V(:,m));
                       else
                           x_error(:,m)=x(:,m) - options.confidenceInterval * sqrt(V(:,m));
                       end
                   end
                   if(mean_x < mean_base)
                       pValue = normcdf(x_error',mean_base',std_base);
                   else
                       pValue = 1-normcdf(x_error',mean_base',std_base);
                   end
                   z_score = (x-mean_base)./std_base;
                   upperBound = z_score + options.confidenceInterval * sqrt(V)./std_base;
                   lowerBound = z_score - options.confidenceInterval * sqrt(V)./std_base;
                   eval(['z_scores.' options.decoders{1} '.' seq.region{i} '(' num2str(k) ').zscore = z_score']);
                   eval(['z_scores.' options.decoders{1} '.' seq.region{i} '(' num2str(k) ').upperBound = upperBound']);
                   eval(['z_scores.' options.decoders{1} '.' seq.region{i} '(' num2str(k) ').lowerBound = lowerBound']);
                end
                eval(['z_scores.' options.decoders{1} '.' seq.region{i} '(' num2str(k) ').pValue = pValue']);
            else
                pValue = (1-normcdf(x',0,1))';
                eval(['z_scores.' options.decoders{1} '.' seq.region{i} '(' num2str(k) ').pValue = pValue']);
                eval(['z_scores.' options.decoders{1} '.' seq.region{i} '(' num2str(k) ').zscore = x']);
            end
        end
    end
    % compute CCF value
    if (size(seq.region,2)==2)          
        baseline = [options.baseline(1)/options.binsize:options.baseline(2)/options.binsize];
        for k=st:et
            xACC_z=eval(['z_scores.' options.decoders{1} '.' seq.region{1} '(' num2str(k) ').zscore;']);
            xACC_lowb=eval(['z_scores.' options.decoders{1} '.' seq.region{1} '(' num2str(k) ').lowerBound;']);
            xACC_upb=eval(['z_scores.' options.decoders{1} '.' seq.region{1} '(' num2str(k) ').upperBound;']);
            xS1_z=eval(['z_scores.' options.decoders{1} '.' seq.region{2} '(' num2str(k) ').zscore;']);
            xS1_lowb=eval(['z_scores.' options.decoders{1} '.' seq.region{2} '(' num2str(k) ').lowerBound;']);
            xS1_upb=eval(['z_scores.' options.decoders{1} '.' seq.region{2} '(' num2str(k) ').upperBound;']);
            xACC=xACC_z;
            xS1=xS1_z;
            for i=1:length(xACC_z)
                temp1 = [xACC_z(i),xACC_lowb(i),xACC_upb(i)];
                [xACC(i),I]=min(abs(temp1));
                xACC(i)=xACC(i)*temp1(I)/abs(temp1(I));
                temp2 = [xS1_z(i),xS1_lowb(i),xS1_upb(i)];
                [xS1(i),I]=min(abs(temp2));
                xS1(i)=xS1(i)*temp2(I)/abs(temp2(I));
%                 xACC(i)=xACC(i)/(xACC_upb(i)-xACC_lowb(i));
            end

            if ~isempty(xS1) && ~isempty(xACC)
                xS1=(xS1+[xS1(1,1) xS1(1,1:end-1)]+[xS1(1,1) xS1(1,2) xS1(1,1:end-2)]+[xS1(1,1) xS1(1,2) xS1(1,3) xS1(1,1:end-3)])./4;
                xACC=(xACC+[xACC(1,1) xACC(1,1:end-1)]+[xACC(1,1) xACC(1,2) xACC(1,1:end-2)]+[xACC(1,1) xACC(1,2) xACC(1,3) xACC(1,1:end-3)])./4;
            end
            ccthresh = 0;
            for m=1:size(xS1,2)
                if abs(xS1(m))>ccthresh
                    xS1(m)=sqrt(abs(xS1(m)))*xS1(m)/abs(xS1(m));
%                     xS1(m)=(ccthresh+(abs(xS1(m)-ccthresh)).^.3)*xS1(m)/abs(xS1(m));
                end
                if abs(xACC(m))>ccthresh
                    xACC(m)=sqrt(abs(xACC(m)))*xACC(m)/abs(xACC(m));
%                     xACC(m)=(ccthresh+(abs(xACC(m)-ccthresh)).^.3)*xACC(m)/abs(xACC(m));
                end
                if(xS1(m)>0),xS1(m)=min(xS1(m),2);else,xS1(m)=max(xS1(m),-2);end
                if(xS1(m)>0),xACC(m)=min(xACC(m),2);else,xACC(m)=max(xACC(m),-2);end
            end
            xx=xACC.*xS1;%.*sqrt(abs(xS1))./abs(xS1);
            if ~isempty(xx)
                cc0=sum(xx(1:10))/10;
                cct=zeros(1,size(xx,2),length(options.rou));
                for rou=1:length(options.rou)
                    cct(1,1,rou)=cc0;                   
                    for tmp=2:size(xx,2)
                        cct(1,tmp,rou)=(1-options.rou(rou))*cct(1,tmp-1,rou)+options.rou(rou)*xx(1,tmp);                        
                    end
                end
                mean_base=mean(cct(1,baseline,:),2);
                std_base=std(cct(1,baseline,:));
                options.C_thresh=[mean_base-options.CCFconfidenceInterval*std_base mean_base+options.CCFconfidenceInterval*std_base];
            else
                mean_base=[];
                std_base=[];
                options.C_thresh=[];
                cct = [];
            end

            eval(['z_scores.' options.decoders{1} '.' seq.region{1} '(' num2str(k) ').ccvalue = cct']);
            eval(['z_scores.' options.decoders{1} '.' seq.region{1} '(' num2str(k) ').ccthresh = options.C_thresh']);
            eval(['z_scores.' options.decoders{1} '.' seq.region{2} '(' num2str(k) ').ccvalue = cct']);
            eval(['z_scores.' options.decoders{1} '.' seq.region{2} '(' num2str(k) ').ccthresh = options.C_thresh']);
        end         
    end
end
