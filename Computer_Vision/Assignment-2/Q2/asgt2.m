x=[110,125,117,135.0277,117.5;
    100,120,110,135.0277,110;
    130,110,120,135.0277,120;
    140,135,137,135.0277,137.5;
    125,130,127,135.0277,127.5]; %the given data is fed into a 2-D maatrix
F=[9 11 13 15 17]; %the attributes of the 'features' are set to a particular combination
glyphplot(x,'Glyph','face','Features',F); %the chernoff faces are plot