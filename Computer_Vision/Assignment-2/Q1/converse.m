a=imread('figure1.jpg');
%imshow(a);
d=imcrop(a,[510 0 300 360]);
%d=imcrop(a,[0 0 300 360]);
%imshow(d);
i=edge(rgb2gray(d));
%imshow(i);

%to plot the polar form of the image.
l=1;
for m=1:360
    for n=1:300
        if i(m,n)==1
            [th(l) r(l)]=cart2pol(m-180,n-150,'bo');
            plot(th(l),r(l));
            if i(m,n)==1
                x=l;
            end
            l=l+1;
            hold on;
        end
    end
end
hold off;

%to re-plot the points with same theta, but onto a different domain.
for t=1:x
    %t=r+1;
    for k=(t+1):x
        if (th(k)==th(t))
            th(k)=th(k)+(2*pi); %the same theta, if there is more than one r, then the theta value of the 2nd r must be added by 360.
        end
        k=k+1;
    end
    t=t+1;
end

%to plot the r-theta values without 2 points of same theta plotted on the
%same domain i.e, n(-pi) to n(pi).
for s=1:x
    plot(th(s),r(s),'b*');
    hold on;
    s=s+1;
end
hold off;

%to check if there are two points with the same theta value
y=0;
for t=1:x
    for k=(t+1):x
        if (th(k)==th(t))
            y=1;
            display(k); %to check the values of theta for which existed more than one edge
        end
        k=k+1;
    end
    t=t+1;
end
display(y); %to display the number of instances redundancy of theta occurs

%to retrieve the image from the polar plot
for i=1:x
    for j=1:x
        if(th(j)>(2*pi))
            th(j)=th(j)-(2*pi);
        end
    end
    [xc yc]=pol2cart(th(i),r(i));
    plot((yc),(-xc)); %to achieve the right orientation of the image, as
    %original
    hold on;
end
hold off;

for i=1:x
    if(th(i)>(2*pi))
        display(i); %to check if any redundant theta values are present
    end
end