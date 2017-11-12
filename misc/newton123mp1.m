function newton123mp1(u0, u1, u2, s, s0, s1, s2, h)
    if nargin < 1
        u0 = 0;
    end
    if nargin < 2
        u1 = 0;
    end
    if nargin < 3
        u2 = 0;
    end
    if nargin < 4
        s = 1;
    end
    if nargin < 5
        h = 1;
    end

    sbar0 = (s + s0)/2;
    sbar1 = (s + s1)/2;
    sbar2 = (s + s2)/2;
    dsbar = [sbar1; sbar2] - sbar0;

    du = [u1; u2] - u0;
    
    sbar = @(x, y) (1 - x - y)*sbar0 + x*sbar1 + y*sbar2;

    M = [1 1; 1 2];
    e = [0; 0];
    f = 1;

    Q = @(x, y) [x y 1]*[M e; e' f]*[x; y; 1];
    l = @(x, y) sqrt(x.^2 + 2.*x.*y + 2.*y.^2 + 1);
    
    g = @(x, y) du + h*(l(x, y)*dsbar + (sbar(x, y)/l(x, y))*(M*[x; y] + e));
    H = @(x, y) h*(2*(dsbar - (sbar(x, y)/(2*Q(x, y)))*(M*[x; y] + e))*(M*[x; ...
                        y] + e)' + sbar(x, y)*M)/l(x, y);

    X(1, :) = [1/3; 1/3];
    
    niter = 50;
    for k = (1:niter) + 1
        x = X(k - 1, 1);
        y = X(k - 1, 2);
        p = H(x, y)\g(x, y);
        X(k, :) = [x; y] - p;
        E(k - 1) = norm(p, 'inf')/norm(X(k, :), 'inf');
        if E(k - 1) < 1e-15
            break;
        end
    end
    if E(length(E)) == 0
        E(length(E)) = eps;
    end
    
    min_ = -1;
    max_ = 2;
    lin = linspace(min_, max_, 121);
    [x y] = meshgrid(lin, lin);
    F = (1 - x - y)*u0 + x*u1 + y*u2 + h*sbar(x, y).*l(x, y);

    figure;
    subplot(1, 2, 1);
    hold on;
    contour(x, y, F, 15);
    plot([0 1 0 0], [0 0 1 0], 'k');
    plot(X(:, 1), X(:, 2), '-*');
    xlim([min_, max_]);
    ylim([min_, max_]);
    scatter(X(size(X, 1), 1), X(size(X, 1), 2), 100);
    subplot(1, 2, 2);
    semilogy(1:length(E), E, '-x');
    xlim([0, length(E) + 1]);
end
