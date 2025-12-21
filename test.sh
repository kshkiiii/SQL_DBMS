H="localhost"
P=7432
q() {
    echo "Запрос: $1"
    echo -n "$1" | nc -N $H $P
    echo ""
}

q "INSERT INTO users VALUES ('Ivan', 25, 'Moscow')"
q "INSERT INTO users VALUES ('Petr', 30, 'Piter')"
q "INSERT INTO posts VALUES ('1', 'Post1')"
q "INSERT INTO posts VALUES ('2', 'Post2')"

q "SELECT users.name, users.city FROM users"
q "SELECT * FROM users WHERE users.age='25'"
q "SELECT users.name, posts.title FROM users, posts WHERE users.users_pk=posts.user_id"
q "DELETE FROM users WHERE users.name='Ivan'"
q "SELECT * FROM users"

Q1="INSERT INTO users VALUES ('L1', 1, 'C1')"
Q2="INSERT INTO users VALUES ('L2', 2, 'C2')"

(echo "Поток 1 ($Q1) отправлен"; echo -n "$Q1" | nc -N $H $P; echo "") &
sleep 0.2
(echo "Поток 2 ($Q2) отправлен"; echo -n "$Q2" | nc -N $H $P; echo "") &

sleep 5
q "SELECT * FROM users"
wait
