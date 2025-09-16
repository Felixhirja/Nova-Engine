#ifndef PLAYER_H
#define PLAYER_H

class Player {
public:
    Player();
    ~Player();

    void Init();
    void Update(double dt);

    double GetX() const;
    double GetY() const;

private:
    double x, y;
    double vx; // velocity in x
};

#endif // PLAYER_H
