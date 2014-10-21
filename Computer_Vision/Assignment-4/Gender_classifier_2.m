function [ gender] = Gender_classifier( x )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
% Face recognition by Santiago Serrano
% Modified by Karoly Pados
% For class use only
close all
clc

% In this example we will only use the first 18
% images for training, and use the 19th image for testing.
% Images 17,18,19 are from the same person,
% and we will se that 19 will have the lowest distance to
% 17 and 18 - meaning it classified correctly
% as the same face as on 17/18.

% number of images on your training set.
M=18;

%Chosen std and mean. 
%It can be any number that it is close to the std and mean of most of the images.
um=100;
ustd=80;

%read and show images(bmp);
S=[];   %img matrix
Gender = [1 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 0 0];
for i=1:M
    str=strcat(int2str(i),'.bmp');   %concatenates two strings that form the name of the image
    eval('img=imread(str);');
    [irow icol]=size(img);    % get the number of rows (N1) and columns (N2)
    temp=reshape(img',irow*icol,1);     %creates a (N1*N2)x1 matrix
    S=[S temp];         %X is a N1*N2xM matrix after finishing the sequence
                        %this is our S
end


%Here we change the mean and std of all images. We normalize all images.
%This is done to reduce the error due to lighting conditions.
for i=1:size(S,2)
    temp=double(S(:,i));
    m=mean(temp);
    st=std(temp);
    S(:,i)=(temp-m)*ustd/st+um;
end

%mean image;
m=mean(S,2);   %obtains the mean of each row instead of each column
tmimg=uint8(m);   %converts to unsigned 8-bit integer. Values range from 0 to 255
img=reshape(tmimg,icol,irow);    %takes the N1*N2x1 vector and creates a N2xN1 matrix
img=img';       %creates a N1xN2 matrix by transposing the image.

% Change image for manipulation
dbx=[];   % A matrix
for i=1:M
    temp=double(S(:,i));
    dbx=[dbx temp];
end

%Covariance matrix C=A'A, L=AA'
A=dbx';
L=A*A';
% vv are the eigenvector for L
% dd are the eigenvalue for both L=dbx'*dbx and C=dbx*dbx';
[vv dd]=eig(L);
% Sort and eliminate those whose eigenvalue is zero
v=[];
d=[];
for i=1:size(vv,2)
    if(dd(i,i)>1e-4)
        v=[v vv(:,i)];
        d=[d dd(i,i)];
    end
 end
 
 %sort, will return an ascending sequence
 [B index]=sort(d);
 ind=zeros(size(index));
 dtemp=zeros(size(index));
 vtemp=zeros(size(v));
 len=length(index);
 for i=1:len
    dtemp(i)=B(len+1-i);
    ind(i)=len+1-index(i);
    vtemp(:,ind(i))=v(:,i);
 end
 d=dtemp;
 v=vtemp;


%Normalization of eigenvectors
for i=1:size(v,2)       %access each column
    v(:,i) = v(:,i)/norm(v(:,i));
end

%Eigenvectors of C matrix
u=[];
for i=1:size(v,2)
    temp=sqrt(d(i));
    u=[u (dbx*v(:,i))./temp];
end

%Normalization of eigenvectors
for i=1:size(u,2)
   u(:,i) = u(:,i)/norm(u(:,i));
end

% Find the weight of each face in the training set.
omega = [];
for h=1:size(dbx,2)
    WW=[];    
    for i=1:size(u,2)
        t = u(:,i)';    
        WeightOfImage = dot(t,dbx(:,h)');
        WW = [WW; WeightOfImage];
    end
    omega = [omega WW];
end


%------------------------------------------------------------
% NOW WE TEST A NEW IMAGE
%------------------------------------------------------------


% Acquire new image
InputImage = imread(x,'bmp');
figure(5)
subplot(1,2,1)
imshow(InputImage); colormap('gray');title('Input image','fontsize',18)
InImage=reshape(double(InputImage)',irow*icol,1);  
temp=InImage;
me=mean(temp);
st=std(temp);
temp=(temp-me)*ustd/st+um;
NormImage = temp;
Difference = temp-m;

p = [];
aa=size(u,2);
for i = 1:aa
    pare = dot(NormImage,u(:,i));
    p = [p; pare];
end
ReshapedImage = m + u(:,1:aa)*p;    %m is the mean image, u is the eigenvector
ReshapedImage = reshape(ReshapedImage,icol,irow);
ReshapedImage = ReshapedImage';

%show the reconstructed image.
subplot(1,2,2)
imagesc(ReshapedImage); colormap('gray');
title('Reconstructed image','fontsize',18)

InImWeight = [];
for i=1:size(u,2)
    t = u(:,i)';
    WeightOfInputImage = dot(t,Difference');
    InImWeight = [InImWeight; WeightOfInputImage];
end

ll = 1:M;
figure(68)
subplot(1,2,1)
stem(ll,InImWeight)
title('Weight of Input Face','fontsize',14)

% Find Euclidean distance
e=[];
for i=1:size(omega,2)
    q = omega(:,i);
    DiffWeight = InImWeight-q;
    mag = norm(DiffWeight);
    e = [e mag];
end

kk = 1:size(e,2);
subplot(1,2,2)
stem(kk,e)
title('Eucledian distance of input image','fontsize',14)

MaximumValue=max(e)
MinimumValue=min(e)

Threshold = (MaximumValue + MinimumValue)/2;

e1 = e;
e1(e1<Threshold)=1;

%disp(e);%Testing values of original distances
%disp(e1);%Testing values of modified distances
closest_face_indexes = find(e1==1);
disp(closest_face_indexes);

male = 0;
female = 0;
[row1 column1] = size(closest_face_indexes);
for j=1:column1
   if(Gender(closest_face_indexes(j)) == 1)
    male = male + 1; %disp('male');disp(male);
   else
       female = female + 1; %disp('female');disp(female);
   end
end
   if(male>female)
       disp('male');
   else disp ('female');
   end
