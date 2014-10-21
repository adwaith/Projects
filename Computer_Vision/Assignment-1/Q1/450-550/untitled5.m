a=imread('0003.jpg'); %read the original image
b=rgb2gray(a); %convert to gray scale
c=edge(b,'canny'); %'Canny' edge detection algorithm
d=edge(b,'log'); %'Laplacian of Gaussian' edge detection algorithm
e=edge(b,'prewitt'); %'Prewitt' edge detection algorithm
f=edge(b,'roberts'); %'Roberts' edge detection algorithm
g=edge(b,'sobel'); %'Sobel' edge detection algorithm
h=edge(b,'zerocross'); %'Zerocross' edge detection algorithm

imshow(h);

ci=imcomplement(c);
di=imcomplement(d);
ei=imcomplement(e);
fi=imcomplement(f);
gi=imcomplement(g);
hi=imcomplement(h);

[c,thresh]=edge(b,'canny',0.09); %original threshold = 0.0188 & 0.0469
%imshow(imcomplement(c));

[d,thresh]=edge(b,'log',0.01); %original threshold = 0.0031
%imshow(imcomplement(d));

[e,thresh]=edge(b,'prewitt',0.11); %original threshold = 0.0960
%imshow(imcomplement(e));

[f,thresh]=edge(b,'roberts',0.05); %original threshold = 0.1117
%imshow(imcomplement(f));

[g,thresh]=edge(b,'sobel',0.07); %original threshold = 0.0978
%imshow(imcomplement(g));

[h,thresh]=edge(b,'zerocross',0.004); %original threshold = 0.0031
%imshow(imcomplement(h));