# Examples
```
fn generate_grid(x, y)
    return NOBOMB:repeat():repeat()

fn modify_tile(grid, f, x, y)
{
    grid[x][y] = grid[x][y] |> f
    return grid
}

fn str_to_int(str)
{
    let num = 0
    let sign = 1

    str = str:filter(fn(c) return c /= 9 and c /= 32)

    if not str:all(fn(c) return 48 <= c and c <= 57 or c == '-')
        throw FormatError

    if str:count(fn(c) return c == '-') > 1
        throw FormatError

    if not str:any(fn(c) return 48 <= c and c <= 57)
        throw FormatError

    if str:head() == '-'
    {
        str = str:tail()
        sign = -1
    }

    for c in str
        num = num * 10 + c

    return sign * num
}

fn nat?(i)
    return i >= 0

fn get_nat()
    return get_line():str_to_int():check(nat?)

fn minesweeper()
{
    let game
    loop
    {
        case game.state
        | STARTING =>
        {
            handle game.x = get_nat() | exception => redo
            handle game.y = get_nat() | exception => redo
            handle game.bombs = get_nat():check(fn(i) return i <= game.x * game.y) | exception => redo
            
            game.grid = generate_grid(game.x, game.y, game.bombs)
        }
        | RUNNING =>
        {
            print(game.grid)

            let choice
            
            handle choice.x = get_nat():check(fn(i) return i <= game.x) | _ => redo
            handle choice.y = get_nat():check(fn(i) return i <= game.y) | _ => redo
            handle
                choice.option = get_line()
                    :filter(fn(c) return whitespace?)
                    :check(fn(s) return s:length == 1)
                    :fn(c)
                    {
                        case c
                        | 'F' => return FLAG
                        | 'M' => return MASS_REVEAL
                        | 'R' => return REVEAL
                        | _   => throw ThisOptionDoesNotExist
                    }
            | exception => redo

            handle case choice.option {
                FLAG        => game.grid = game.grid:toggle_flag(game.x, game.y)
                REVEAL      => game.grid = game.grid:reveal(game.x, game.y)     
                MASS_REVEAL => game.grid = game.grid:mass_reveal(game.x, game.y)
                _           => throw CantHandleOption
            }
            | exception => continue
            | bomb_found =>
                {
                    print(game.grid)
                    print("Game over, start again?)

                    handle choice.option = get_line()
                        :filter(fn(c) return whitespace?)
                        :check(fn(s) return s:length == 1)
                        :uppercase
                        :fn(c)
                        {
                            case c
                            | 'Y' => game.state = STARTING
                            | 'N' => game.state = ENDED
                            | _   => throw ThisOptionDoesNotExist
                        }
                    | exception => redo
                }

        }
        | ENDED => break
    }
}

```
