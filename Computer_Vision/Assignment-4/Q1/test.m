X=[0 0;5 0;0 5;5 5];
D=pdist(X,'cosine');
display(D);

%In the euclidean method of determining distances, the absolute distance
%between the points in the cartesian plane are evlauated. However, in the
%cosine method, one minus the angle between the vectors formed with each
%point (with respect to the origin) is obtained. The values are computed in
%every possible combination of points. 