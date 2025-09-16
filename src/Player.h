#ifndef PLAYER_H
#define PLAYER_H

class Player {
public:
    Player();
    ~Player();

    void Init();
    // Update with input: left/right boolean input
    void Update(double dt, bool inputLeft = false, bool inputRight = false);

    double GetX() const;
    double GetY() const;

private:
    double x, y;
    double vx; // velocity in x
    double ax; // acceleration
};

#endif // PLAYER_H
