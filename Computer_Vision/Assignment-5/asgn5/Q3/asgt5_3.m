workingDir = pwd;
imageNames = dir(fullfile(workingDir, 'images', '*.jpg'));
imageNames = {imageNames.name}';
% Set up output video
outputVideo = VideoWriter(fullfile(workingDir,'videoq3_15'));
outputVideo.FrameRate = 15;
open(outputVideo);
for ii = 1:length(imageNames)
 img = imread(fullfile(workingDir,'images',imageNames{ii})); % read the image
 writeVideo(outputVideo,img); % Write to video
end
close(outputVideo);