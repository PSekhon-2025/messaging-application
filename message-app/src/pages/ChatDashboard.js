import "./ChatDashboard.css";
import React from "react";
import { useNavigate } from "react-router-dom";

const users = ["Room 1", "Room 2", "Room 3", "Room 4", "Room 5", "Room 6", "Room 7"];

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
