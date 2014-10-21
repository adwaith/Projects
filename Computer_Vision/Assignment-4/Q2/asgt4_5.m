a=[2, 6, 10, 9, 5, 9, 8, 4, 5, 3]; %the given values are fed as input
b=linkage(a,'average'); %the linkage matrix is formed
dendrogram(b,5) %the desired dendrogram is obtained