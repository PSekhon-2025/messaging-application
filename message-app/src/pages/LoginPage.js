import "./LoginPage.css";
import React, { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";

const LoginPage = () => {
  const [isSignup, setIsSignup] = useState(false);
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const navigate = useNavigate();
  const [socket, setSocket] = useState(null);

  useEffect(() => {
    // Establish a WebSocket connection to the server
    const ws = new WebSocket("ws://localhost:9000");
    ws.onopen = () => {
      console.log("Connected to the server");
    };
    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.type === "login_response") {
        if (data.status === "success") {
          navigate("/dashboard");
        } else {
          setError(data.message);
        }
      } else if (data.type === "signup_response") {
        setError(data.message);
        if (data.status === "success") {
          setIsSignup(false);
          setUsername("");
          setPassword("");
        }
      }
    };
    ws.onerror = (err) => {
      console.error("WebSocket error:", err);
      setError("WebSocket connection error.");
    };
    setSocket(ws);

    return () => {
      ws.close();
    };
  }, [navigate]);

  const handleSubmit = () => {
    setError(""); // Clear any previous errors

    if (!username.trim() || !password.trim()) {
      setError("Username and password are required.");
      return;
    }

    if (!socket || socket.readyState !== WebSocket.OPEN) {
      setError("Not connected to server.");
      return;
    }

    if (isSignup) {
      // Send a signup message to the server
      const signupMessage = {
        type: "signup",
        username,
        password,
      };
      socket.send(JSON.stringify(signupMessage));
    } else {
      // Send a login message to the server
      const loginMessage = {
        type: "login",
        username,
        password,
      };
      socket.send(JSON.stringify(loginMessage));
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
