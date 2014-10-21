obj=VideoReader('store1(1).mp4');% read a video file
n=obj.NumberOfFrames;
objHeight=obj.Height;
objWidth=obj.Width;
for i=1:30% read a sequence of 30 images
    img(i).cdata=read(obj,i);
end

%imshow(img(1).cdata)
%rectangle('position',[objWidth/4,objHeight/4,objWidth/2,objHeight/2],'LineWidth',2,'EdgeColor','r');% a red rectangle box is added to each of the 30 images

workingDir = pwd;
imageNames = dir(fullfile(workingDir, 'images', '*.jpg'));
imageNames = {imageNames.name}';
% Set up output video
outputVideo = VideoWriter(fullfile(workingDir,'newvideo'));
outputVideo.FrameRate = 4;
open(outputVideo);
for ii = 1:length(imageNames)
 img = imread(fullfile(workingDir,'images',imageNames{ii})); % read the image
 writeVideo(outputVideo,img); % Write to video
end
close(outputVideo);