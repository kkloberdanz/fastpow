import math


# TODO: generate lookup table for guesses, based on truncated floating point
#       numbers as the index
def mylog(x, guess=x/10):
    '''
    instead of solving for ln(x) directly, solve for
        f(y) = y - ln(x)
        f(y) = exp(y) - exp(ln(x))
        f(y) = exp(y) - x

        f'(y) = exp(y)

        f(y) / f'(y) = 1 - x / exp(x)
    '''

    close_to_zero = 1e-15
    if abs(x) < close_to_zero:
        return 1

    y_new = guess
    i = 0
    while True:
        i += 1
        y_old = y_new
        y_new = y_new - 1 + x / math.exp(y_new)
        print(y_new)
        if abs(y_old - y_new) < close_to_zero:
            print(f"converged after {i} iterations")
            break
    if abs(y_new) < close_to_zero:
        return 0
    else:
        return y_new
