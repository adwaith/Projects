input_vid = VideoReader('store1(1).mp4'); %read the video from file
no_frames = input_vid.NumberOfFrames; %compute the number of frames
stable_bg = read(input_vid, 1); %store the background image as the first frame of the video
stable_bg_gr = rgb2gray(stable_bg); %convert the background to grayscale
[height width] = size(stable_bg_gr); %calculate the dimensions of the background
foreground = zeros(height, width); %calculate the dimensions of the foreground with zeros function
average = zeros(height, width); %compute the average height and width
threshold = 20; %set a threshold

for i = 2:no_frames %traverse from the second frame to the last frame of the video
    
     current_frame = read(input_vid, i); %read the 'i'th frame
     current_frame_gr = rgb2gray(current_frame); %convert the frame to grayscale
     delta = abs(double(current_frame_gr) - double(stable_bg_gr)); %compute the difference between the absolute value and type casted double value

     for j = 1:width %traverse through the pixels of the width
         
         for k = 1:height %traverse through the pixels of the height

             if (delta(k, j) > threshold) %compare the difference with the threshold
                foreground(k, j) = current_frame_gr(k, j); %if the difference is greater, set the current frame accordingly
             else 
                 foreground(k, j) = 0; %otherwise zero out the foreground
             end 

         end 
     end 
     average = average + foreground; %update the average
     figure(1), subplot(2, 2, 1), imshow(current_frame); %plot the input image i.e., the current frame 
     title('Input Image') 
     subplot(2, 2, 3), imshow(uint8(stable_bg_gr)) %display the grayscale background 
     title('Background Model') 
     subplot(2, 2, 4), imshow(uint8(foreground)) %display the foreground
     title('Foreground') 

     stable_bg_gr = current_frame_gr; %update the background by assigning the current frame to it

end 
 
result = average/no_frames; %let the resultant image be the average image array of all frames
figure(2), imshow(result); %display the motion energy image
title('Motion Energy Image');
