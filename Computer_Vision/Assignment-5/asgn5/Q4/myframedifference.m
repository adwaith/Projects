% Frame difference method
obj = VideoReader('/Users/Adwaith/18-798/asgn5/Q4/store1(1).mov');
no_frames = get(obj, 'numberOfFrames');
obj2 = VideoWriter('/Users/Adwaith/18-798/asgn5/Q4/q4_video1');
obj2.FrameRate = 5;

open(obj2);
threshold = 10;           
background = read(obj,1);% The first frame of the movie is read as the background
background_bw = rgb2gray(background);% the background is converted to grayscale
frame_size = size(background);             
objwidth = frame_size(2);
objheight = frame_size(1);
foreground = zeros(objheight, objwidth);

for i = 2:no_frames
    
    fr = read(obj,i);% every frame of the movie is read
    frame_bw = rgb2gray(fr);% each frame is converted to grayscale
    
    frame_diff = abs(double(frame_bw) - double(background_bw));% negative values are prevented by taking the absolute
    
    for j=1:objwidth% checking if frame_difference > threshold pixel in foreground
        for k=1:objheight
            if ((frame_diff(k,j) > threshold))
                foreground(k,j) = frame_bw(k,j);
            else
                foreground(k,j) = 0;
            end
        end
    end
    
    background_bw = frame_bw;
          
    writeVideo(obj2,uint8(foreground));% the resultant set of frames are written into a video format
  
end
figure, title('Contour of extracted objects using Frame difference filter');
imcontour(foreground);
hold on;
close(obj2);

 
    