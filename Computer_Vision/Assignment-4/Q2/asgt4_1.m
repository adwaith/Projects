x = input('Feed array X: ');
y = input('Feed array Y: ');
distance = 0;
numerator = 0;
denominator1 = 0;
denominator2 = 0; 
if length(x)<length(y) %to take the shorter length arrays between the two
    l=length(x);
elseif length(x)>length(y)
        l=length(y);
else
    l=length(x);
end     
for i = 1:l
    distance = distance + (x(i) - y(i))^2; %finding the square of the distance between the two array points
end
euclidean_dist = sqrt(distance); %taking square root of the 'distance', we arrive at the euclidean distance
for i = 1:l  
    numerator = numerator + (x(i) * y(i)); %the numerator is a cumulative addition of the product of corresponding elemts of the given arrays
    denominator1 = denominator1 + x(i)^2; %the denominators are seperately calculated from the array elements
    denominator2 = denominator2 + y(i)^2;
end 
cosine_angle_coefficient = numerator / (sqrt(denominator1) * sqrt(denominator2)); %the cosine angle coefficient is calculated
display(euclidean_dist);
display(cosine_angle_coefficient);