import "./ChatDashboard.css";
import React from "react";
import { useNavigate } from "react-router-dom";

const users = ["test", "test1", "test2", "test3", "test4", "test5", "test6"];

const ChatDashboard = () => {
  const navigate = useNavigate();
  const currentUser = localStorage.getItem("username");

  const handleChatClick = (username) => {
    navigate(`/chat/${username}`);
  };

  return (
    <div className="dashboard-container">
      <div className="dashboard-header">
        <h2>Welcome, {currentUser || "User"} 👋</h2>
        <p>Select someone to start chatting</p>
      </div>

      <div className="user-list">
        {users
          .filter((user) => user !== currentUser)
          .map((user) => (
            <div
              key={user}
              className="user-card"
              onClick={() => handleChatClick(user)}
            >
              <h4>{user}</h4>
            </div>
          ))}
      </div>
    </div>
  );
};

export default ChatDashboard;
