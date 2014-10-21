img=imread('1.jpg');
right=imcrop(img,[0 0 360 479]);
left=imcrop(img,[360 0 360 479]);
right(:,:,3)=0;
left(:,:,1)=0;
left(:,:,2)=0;
imshow(right);
img_3D=imfuse(left,right);
imshow(img_3D);