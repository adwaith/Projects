obj=VideoReader('store1(1).mp4');% read a video file
n=obj.NumberOfFrames;
objHeight=obj.Height;
objWidth=obj.Width;
for i=1:30% retrieve 30 frames from the video
    img(i).cdata=read(obj,i);
end
imshow(img(1).cdata);

for i=1:6% concatenate a 6*5 matrix of images using horzcat and vertcat
    image1=horzcat(img(1).cdata,img(2).cdata,img(3).cdata,img(4).cdata,img(5).cdata,img(6).cdata);
end
for i=1:6
    image2=horzcat(img(7).cdata,img(8).cdata,img(9).cdata,img(10).cdata,img(11).cdata,img(12).cdata);
end
for i=1:6
    image3=horzcat(img(13).cdata,img(14).cdata,img(15).cdata,img(16).cdata,img(17).cdata,img(18).cdata);
end
for i=1:6
    image4=horzcat(img(19).cdata,img(20).cdata,img(21).cdata,img(22).cdata,img(23).cdata,img(24).cdata);
end
for i=1:6
    image5=horzcat(img(25).cdata,img(26).cdata,img(27).cdata,img(28).cdata,img(29).cdata,img(30).cdata);
end
image_final=vertcat(image1,image2,image3,image4,image5);
imshow(image_final);
