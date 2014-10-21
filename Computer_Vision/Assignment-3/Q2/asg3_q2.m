b=imread('image1.jpg'); 
prompt='Feed a message      ';
c=input(prompt,'s'); %to take a string input
clen=length(c);
blen=length(b);
j=1;
m=1;
o=0;
%encoding the input string by replacing the LSB or pixels of the image
for k=1:clen
    str=dec2bin(c(k),8);
    for i=m:(m+7)
        if i>blen
            o=o+1;
            i=i-blen;
        end
        str1=dec2bin(b((j+(blen*o/blen)),i,1),8);
        str1(8)=str((mod((i-1),8)+1));
        b((j+(blen*o/blen)),i,1)=bin2dec(str1);
        i=i+1;
    end
    m=m+8;
    k=k+1;
end 
imshow(b);

%decoding the hidden message from the original picture
j=1;
m=1;
o=0;
n=1;
for i=0:(clen-1)
    for k=1:8
        z=(i*8)+k;
        if (z>blen)
            z=z-blen;
            j=j+1;
        end
        b_str1=dec2bin(b(j,z,1),8);
        rstr((i+1),k)=b_str1(8);
        k=k+1;
    end
    i=i+1;
end
display('Decoded Message:');
for i=1:clen
    display(char(bin2dec(rstr(i,:))));
end