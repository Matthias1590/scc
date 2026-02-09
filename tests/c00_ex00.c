/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   c00_ex00.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mawijnsm <mawijnsm@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 11:37:07 by mawijnsm          #+#    #+#             */
/*   Updated: 2026/02/09 15:38:44 by mawijnsm         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

int write(int fd, char *c, int count);

void	ft_putchar(char c)
{
	write(1, &c, 1);
}

int main()
{
    ft_putchar('4');
    ft_putchar('2');
    ft_putchar('\n');

    return 0;
}
