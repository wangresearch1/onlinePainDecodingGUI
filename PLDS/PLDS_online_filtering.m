
%
% add var output
% hsl 2016-11-29
function [state, var] = PLDS_online_filtering(params,y, z0,Q0,binsize) 

if nargin < 5; 
    binsize = 0.05; % 50 ms
end
if nargin < 4; 
    Q0 = params.model.Q0;    
end
if nargin < 3; 
    z0 = params.model.x0;    
end

% z0 is the latent state estimate from the last time point prior to y(1:T) 
% Q0 is the latent state variance from the last time point prior to y(1:T)

A = params.model.A; % m-by-m
C = params.model.C; % C-by-m
d = params.model.d; % C-by-1

Q = params.model.Q; % m-by-m

add_nosie = 0;
if add_nosie
   for j=180:10:260
%        r = randi([1 10],1,4);
       for i=1:10
%            y(r(i),j)=y(r(i),j)+2;
        y(i,j)=y(i,j)+3;
       end
   end
end

 
[szC,T] = size(y);
% when z0 is univariate, m=1
m = size(z0,1); 
state = zeros(m,T);
var = zeros(m,T);

%for test
%C = [-0.22701;0.0414355;0.025736;0.137555;0.0675383;0.24028;0.0689439];
%d = [-0.32909;-1.45425;-0.801732;-0.931861;-0.98515;-1.1376;-0.709358];
% z0 = -0.508155;
% Q0 = 1.57267;
% A = 0.991369;
% Q = 0.170969;
% for test
y_diff_rec = [];
Q_filt_rec = [];
y_pred_rec = [];
z_pred_rec = [];
Q_pred_rec = [];
yy_rec = [];

% yy = (log(y(:,1)/binsize + 0.1)-d);
% z0 = (yy'*C)/(C'*C);

% assuming single-trial structure 
for t=1:T
    % prediction
    z_pred = A * z0; % m-by-1
    Q_pred = A * Q0 * A' + Q; % m-by-m
    %rate = exp(C * x_pred + d); % C-by-1
    rate = exp(C * z_pred + d); % C-by-1 % ms是z_pred写成了x_pred？
    y_pred = rate * binsize; % C-by-1 
    
    % filtering
    invQ = inv(Q_pred) + (C' * diag(y_pred) * C); %m-by-m
    Q_filt = inv(invQ);    % m-by-m
    z_filt = z_pred + Q_filt * (C' * (y(:,t) - y_pred));  % m-by-1 
    y_diff_rec = [y_diff_rec,(C' * (y(:,t) - y_pred))];
    Q_filt_rec = [Q_filt_rec Q_filt];
    y_pred_rec = [y_pred_rec y_pred];
    z_pred_rec = [z_pred_rec z_pred];
    Q_pred_rec = [Q_pred_rec Q_pred];
    if t>1 && sum(y(:,t))>0
        yy_rec = [yy_rec  (abs(C')*y(:,t))/(abs(C')*y(:,t-1))];
    end
    
%     z_filt = z_pred + Q_pred * (C' * (y(:,t) - y_pred));  % m-by-1
%     rate = exp(C * z_filt + d); % C-by-1 % ms是z_pred写成了x_pred？
%     y_pred = rate * binsize; % C-by-1
%     invQ = inv(Q_pred) + (C' * diag(y_pred) * C); %m-by-m
%     Q_filt = inv(invQ);    % m-by-m
    

    % save result 
    state(:,t) = z_filt;
    var(:,(1:m)*t) = Q_filt;
    z0 = z_filt;
    Q0 = Q_filt;% why Q0 update missed?
end
stop = 0;