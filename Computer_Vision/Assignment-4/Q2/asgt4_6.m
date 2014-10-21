x=-3:0.05:6; %Initializing the range of 'x' values
y = gaussmf(x, [3 3]); %plotting the guassian curve
plot(x,y); 
xlabel('gaussmf, P=[3 3]')
ylabel('Mapped Values')