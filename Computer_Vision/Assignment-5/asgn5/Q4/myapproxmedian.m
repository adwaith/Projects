% Approximate Median filter method
obj = VideoReader('/Users/Adwaith/18-798/asgn5/Q4/store1(1).mov');
no_frames = get(obj, 'numberOfframes');
obj2 = VideoWriter('/Users/Adwaith/18-798/asgn5/Q4/q4_video');
obj2.FrameRate = 5;

open(obj2);
threshold = 25;           
background = read(obj,1);% The first frame of the movie is read as the background
background_bw = double(rgb2gray(background));% the background is converted to grayscale
frame_size = size(background);             
objwidth = frame_size(2);
objheight = frame_size(1);
foreground = zeros(objheight, objwidth);

for i = 2:no_frames
    frame = read(obj,i);% every frame of the movie is read
    frame_bw = rgb2gray(frame);% each frame is converted to grayscale
    
    frame_difference = abs(double(frame_bw) - double(background_bw));% negative values are prevented by taking the absolute

    for j=1:objwidth% checking if frame_difference > threshold pixel in foreground
         for k=1:objheight

             if ((frame_difference(k,j) > threshold))
                 foreground(k,j) = frame_bw(k,j);
             else
                 foreground(k,j) = 0;
             end

             if (frame_bw(k,j) > background_bw(k,j))          
                 background_bw(k,j) = background_bw(k,j) + 1;           
             elseif (frame_bw(k,j) < background_bw(k,j))
                 background_bw(k,j) = background_bw(k,j) - 1;     
             end

         end    
    end
         
     writeVideo(obj2,uint8(foreground));% the resultant set of frames are written into a video format
    
end
title('Contour of extracted objects using Approximate median filter');
imcontour(foreground);
hold on;
close(obj2);
 