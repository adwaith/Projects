function [ stats ] = calc_stats( image )
% This function calculates texture statistics from a given image, or pixel
% block
% Initialize the output vector
stats = zeros(1,9);
% Get the histogram
[pix_count, pix_val] = imhist(image);
% Normalise the histogram to get pixel-intensity probability
pix_prob = pix_count / sum(pix_count);
% Get the mean of the pixel value
img_mean = mean2(image);
% Get the variance of the pixel values
img_var = (std2(image))^2;
% Precompute
diff = pix_val - img_mean;
% Get the skewness of the histogram
img_skew = (diff' .* diff') * (diff .* pix_prob);
% Get the flatness of the histogram
img_flatness = (diff' .* diff' .* diff') * (diff .* pix_prob);
% Get the uniformity of the histogram
img_uniformity = pix_prob' * pix_prob;
% Get the entropy of the image
img_entropy = entropy(image);
% Get the gray-level co-occurrence matrix
img_glcm = graycomatrix(image,'NumLevels', 256);
% Get the image contrast
img_contrast = graycoprops(uint8(img_glcm), 'Contrast');
% Get the image correlation
img_correlation = graycoprops(img_glcm, 'Correlation');
% Get the image energy
img_energy = graycoprops(img_glcm, 'Energy');
% Get the image homogeneity
img_homogeneity = graycoprops(img_glcm, 'Homogeneity');
stats(1) = img_var;
stats(2) = img_skew;
stats(3) = img_flatness;
stats(4) = img_uniformity;
stats(5) = img_entropy;
stats(6) = img_contrast.Contrast;
stats(7) = img_correlation.Correlation;
stats(8) = img_energy.Energy;
stats(9) = img_homogeneity.Homogeneity;
end