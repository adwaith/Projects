function [ result ] = feature_graycoprops( img_gray, offset )

[M N ~] = size(img_gray);

% Calculate co-occurrence matrix (GLCM)
%D = [floor(M/10) floor(N/10)];
D = offset;
cm = graycomatrix(img_gray, 'Offset', D, 'NumLevels', 64);

% Calculate homogeneity and energy
props = graycoprops(cm, {'homogeneity', 'energy', 'contrast', 'Correlation'});
%props = graycoprops(cm, 'energy');
energy = props.Energy;
homo = props.Homogeneity;
contrast = props.Contrast;
correlation = props.Correlation;

% Normalize GLCM
cm = cm ./ sum(sum(cm));

% Calculate entropy
[M N] = size(cm);
entropy = 0;
for x = 1:N
    for y = 1:M
        if (cm(y,x) > 0)
            entropy = entropy + cm(y,x)*log(cm(y,x));
        end
    end
end

% Assign output
result = [-entropy homo energy contrast correlation];
%result = [-entropy  energy];

end

