import "./LoginPage.css";
import React, { useState } from "react";
import { useNavigate } from "react-router-dom";

const LoginPage = () => {
  const [isSignup, setIsSignup] = useState(false);
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const navigate = useNavigate();

  const handleSubmit = () => {
    setError(""); // Clear previous errors

    if (!username.trim() || !password.trim()) {
      setError("Username and password are required.");
      return;
    }

    const users = JSON.parse(localStorage.getItem("users") || "{}");

    if (isSignup) {
      if (users[username]) {
        setError("Username already taken.");
        return;
      }
      users[username] = password;
      localStorage.setItem("users", JSON.stringify(users));
      setError("User registered successfully!");
      setIsSignup(false);
      setUsername("");
      setPassword("");
    } else {
      if (!users[username]) {
        setError("Invalid username.");
      } else if (users[username] !== password) {
        setError("Invalid password.");
      } else {
        localStorage.setItem("username", username);
        navigate("/dashboard");
      }
    }
  };

  return (
    <div className="login-container">
      <div className="login-card">
        <h2 className="login-title">{isSignup ? "Sign Up" : "Login"}</h2>

        <input
          className="login-input"
          placeholder="Username"
          value={username}
          onChange={(e) => setUsername(e.target.value)}
        />

        <input
          className="login-input"
          type="password"
          placeholder="Password"
          value={password}
          onChange={(e) => setPassword(e.target.value)}
        />

        {error && (
          <p className={error.includes("successfully") ? "login-success" : "login-error"}>
            {error}
          </p>
        )}

        <button className="login-button" onClick={handleSubmit}>
          {isSignup ? "Sign Up" : "Login"}
        </button>

        <button
          className="login-switch-button"
          onClick={() => {
            setIsSignup(!isSignup);
            setError("");
            setUsername("");
            setPassword("");
          }}
        >
          {isSignup ? "Already have an account? Login" : "Don't have an account? Sign Up"}
        </button>
      </div>
    </div>
  );
};

export default LoginPage;
