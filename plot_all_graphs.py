import matplotlib.pyplot as plt

# Read all data
def read_data(filename):
    data = []
    with open(filename, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) == 2:
                data.append((float(parts[0]), float(parts[1])))
    return data

density_data = read_data('density_data.txt')
requester_data = read_data('requester_data.txt')
resource_types_data = read_data('resource_types_data.txt')

# Create 2x2 subplot
fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 10))

# Graph (a): Density
densities = [d[0] for d in density_data]
success_a = [d[1] for d in density_data]
ax1.plot(densities, success_a, 'o-', linewidth=2, markersize=8)
ax1.set_xlabel('Vehicle Density (vehicles/km²)')
ax1.set_ylabel('Success Ratio (%)')
ax1.set_title('(a) Vehicle Density Impact')
ax1.grid(True, alpha=0.3)

# Graph (b): Requester Ratio
ratios = [d[0] for d in requester_data]
success_b = [d[1] for d in requester_data]
ax2.plot(ratios, success_b, 's-', linewidth=2, markersize=8, color='orange')
ax2.set_xlabel('Requester Ratio (%)')
ax2.set_ylabel('Success Ratio (%)')
ax2.set_title('(b) Requester Ratio Impact')
ax2.grid(True, alpha=0.3)

# Graph (c): Empty (Speed - skipped)
ax3.text(0.5, 0.5, '(c) Vehicle Speed\n(Future Enhancement)', 
         ha='center', va='center', fontsize=12)
ax3.axis('off')

# Graph (d): Resource Types
types = [d[0] for d in resource_types_data]
success_d = [d[1] for d in resource_types_data]
ax4.bar(types, success_d, width=0.6, color='green', alpha=0.7)
ax4.set_xlabel('Number of Resource Types')
ax4.set_ylabel('Success Ratio (%)')
ax4.set_title('(d) Resource Types Impact')
ax4.set_xticks(types)
ax4.grid(True, alpha=0.3, axis='y')

plt.tight_layout()
plt.savefig('Figure_5_RCSA_Performance.png', dpi=300, bbox_inches='tight')
print("✅ Combined graph saved: Figure_5_RCSA_Performance.png")
plt.close()
