input_vid = VideoReader('store1(1).mp4'); %read the video from file
no_frames = input_vid.NumberOfFrames; %compute the number of frames
stable_bg = read(input_vid, 1); %store the background image as the first frame of the video
stable_bg_gr = rgb2gray(stable_bg); %convert the background to grayscale
[height, width] = size(stable_bg_gr); %calculate the dimensions of the background
foreground = zeros(height, width); %calculate the dimensions of the foreground with zeros function
mhi = zeros(height, width); %compute the average height and width
 
speed = 1.5; %setting the decremental value for updating the motion history image
pause on;
threshold = 20; %set a threshold
 
for i = 2:100; %traverse from the second frame to the last frame of the video
   
    current_frame = read(input_vid, i); %read the 'i'th frame
    current_frame_gray = rgb2gray(current_frame); %convert the frame to grayscale
    
    delta = abs(double(current_frame_gray) - double(stable_bg_gr)); %compute the difference between the absolute value and type casted double value
    
    for j = 1: width %traverse through the pixels of the width
        
        for k = 1:height %traverse through the pixels of the height
            
            if (delta(k, j) > threshold) %compare the difference with the threshold
                mhi(k, j) = current_frame_gray(k, j); %if the difference is greater, set the motion history image accordingly
            else
                if (mhi(k, j) ~= 0) %check if the array value is not equal to zero
                    mhi(k,j) = mhi(k,j) - speed; %if true, decrement the value by 'speed'
                    foreground(k, j) = mhi(k, j); %subsequently, set the foreground
                else
                    foreground(k, j) = 0; %if not true, set the array value to zero
                end
            end
        end
    end
            
    fg_filtered = medfilt2(foreground, [4, 4]);
    
    figure(1), subplot(2, 2, 1), imshow(current_frame);
    title('Input Image');
    
    subplot(2, 2, 3), imshow(uint8(stable_bg_gr))
    title('Background');
    
    colormap('hot'); %plotting the heat map of the image
 
    subplot(2, 2, 4), imagesc(foreground); 
    title('Heat Map');

    stable_bg_gr = current_frame_gray; %update the background by assigning the current frame to it
    
end
 
figure (2), imagesc(foreground);
title('Heat Map');
