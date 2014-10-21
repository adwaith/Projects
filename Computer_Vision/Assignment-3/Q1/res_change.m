%to change the image resolution
a = imread('Lines_1024-819_72-72.png');
I=rgb2gray(a);
for j=1:819
    for i=1:1024
        I(j,i)=I(j,i)/2;
    end
end
imshow(I);