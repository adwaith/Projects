% Co-Occurrence Based Model Texture Analysis
 
% Read pictures from the Van Gogh/

for n=1:8
    number=strcat(num2str(n),'.jpg');
    imageName=strcat('Van Gogh/', number);
    image = imread(imageName);
    image=rgb2gray(image);
    [glcms,SI]=graycomatrix(image);
    % get the Homogeneity Energy Contrast Correlation
    stats1(n)= graycoprops(glcms,'Homogeneity Energy Contrast Correlation');
end


% Read pictures from the Van Gogh/
for n=1:8
    number=strcat(num2str(n),'.jpg');
    imageName=strcat('Georges Seura/', number);
    image = imread(imageName);
    image=rgb2gray(image);
    [glcms,SI]=graycomatrix(image);
     % get the Homogeneity Energy Contrast Correlation
    stats2(n)= graycoprops(glcms,'Homogeneity Energy Contrast Correlation');
end