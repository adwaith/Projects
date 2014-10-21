close all; 
clear; 
 

thresh = 30; 

inputReader = VideoReader('rocketshort.mp4'); 
numOfFrames = inputReader.NumberOfFrames; 
stable_bg = read(inputReader, 1); 
stable_bg_gr = rgb2gray(stable_bg); 
[height width] = size(stable_bg_gr); 
foreground = zeros(height, width); 


for i = 2:numOfFrames 
     %current_frame = step(hbfr); 
     current_frame = read(inputReader, i); 
     current_frame_gr = rgb2gray(current_frame); 

     delta = abs(double(current_frame_gr) - double(stable_bg_gr)); 

     for j = 1:width 
         for k = 1:height 

             if (delta(k, j) > thresh) 
                foreground(k, j) = current_frame_gr(k, j); 
             else 
                 foreground(k, j) = 0; 
             end 

         end 
     end 

     figure(1), subplot(2, 2, 1), imshow(current_frame); 
     title('Input Image') 
     subplot(2, 2, 3), imshow(uint8(stable_bg_gr)) 
     title('Background Model') 
     subplot(2, 2, 4), imshow(uint8(foreground)) 
     title('Foreground') 

     stable_bg_gr = current_frame_gr; 

end 

