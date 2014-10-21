img=imread('1.jpg'); %Read a stereo image
right=imcrop(img,[0 0 360 479]); %Crop the right half 
left=imcrop(img,[360 0 360 479]); %Crop the left half
right(:,:,1)=0; %Set the right channel's red component to zero i.e., cyan
left(:,:,2)=0; %Set the left channel's green component to zero
left(:,:,3)=0; %Set the left channel's blue component to zero
imshow(right);
imshow(left);
img_3D=imfuse(left,right); %Overlap the left and right channels to get the 3D image
imshow(img_3D);