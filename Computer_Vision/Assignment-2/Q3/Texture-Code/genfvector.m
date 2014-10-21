%generate feature vectors

function fvector = genfvector(image)

    image = squeeze(image);
    % Gray level histogram based analysis of textures

    [H W D] = size(image);
    if(D>1)
        img = rgb2gray(image);
    end
    v = [0:255]';
    h = imhist(img);
    N = double(h./(H*W));

    mean = v.*N;
    mean = sum(mean);
    mean = mean/255;

    adj = double(N-mean);

    %Second order moment (Variance)
    m2 = (adj.*adj);
    m2 = m2.*N;
    m2 = sum(m2)*255;

    %Standard Deviation
    sd = sqrt(m2);

    %Third order moment (Skew)
    m3 = (adj.*adj.*adj);
    m3 = m3.*N;
    m3 = sum(m3);

    %Fourth order moment (Flatness)
    m4 = (adj.*adj.*adj.*adj);
    m4 = m4.*N;
    m4 = sum(m4);

    %Uniformity
    U = sum(N.*N);

    %Entropy 
    %NB: N(p)log2(N(p)) for N(p)=0 then log2(0)=Inf however since this is multiplied by 0 again the formula is valid
    % Matlab cannot handle mutliplying by infinity therefore we must account for cases where N(p) = 0)
    e = N;
    for(p=1:256)
        if(N(p)>0)
            e(p) = log2(N(p))*N(p);
        else
            e(p) = 0;
        end
    end
    e = -sum(e);

    %check uint8
    gcomatrix = graycomatrix(rgb2gray(squeeze(uint8(...
        image))),'NumLevels', 256);
    params = graycoprops(gcomatrix,'all');
    %could calculate those other 3 things as part of the feature vectors
    fvector = [params.Contrast/((size(gcomatrix,1)-1)^2), ...
        params.Correlation/2, params.Homogeneity]; %sure entropy....

end